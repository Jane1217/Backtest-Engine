#!/usr/bin/env python3
"""
Backtest Engine Web Interface
Simple Flask backend providing Web API
"""

from flask import Flask, render_template, request, jsonify, send_file
import subprocess
import os
import json
import csv
from pathlib import Path
import time

app = Flask(__name__, template_folder='templates', static_folder='static')
# Results directory is in project root
RESULTS_DIR = Path(__file__).parent.parent / "results"

@app.route('/')
def index():
    """Main page"""
    return render_template('index.html')

@app.route('/api/run', methods=['POST'])
def run_backtest():
    """Run backtest"""
    try:
        data = request.json
        num_ticks = int(data.get('num_ticks', 1000))
        initial_capital = float(data.get('initial_capital', 10000))
        strategies = data.get('strategies', ['Mean_Reversion', 'Breakout_Win20', 'Spread'])
        
        # Ensure results directory exists
        RESULTS_DIR.mkdir(exist_ok=True)
        
        # Create temporary directory for this backtest run
        import tempfile
        import shutil
        temp_dir = tempfile.mkdtemp(prefix='backtest_')
        temp_results_dir = Path(temp_dir) / 'results'
        temp_results_dir.mkdir(exist_ok=True)
        
        # Run Docker container to execute backtest
        # CSV files will be generated in temp_results directory, not in project results/
        # Pass parameters via environment variables so C++ code can read them
        cmd = [
            'docker', 'run', '--rm',
            '-v', f'{temp_results_dir.absolute()}:/app/temp_results',
            '-w', '/app',
            '-e', f'NUM_TICKS={num_ticks}',
            '-e', f'INITIAL_CAPITAL={initial_capital}',
            '-e', 'WEB_INTERFACE=1',  # Disable verbose console output
            'backtest-engine',
            'sh', '-c',
            './build/BacktestEngine && (mv *.csv temp_results/ 2>/dev/null || true)'
        ]
        
        start_time = time.time()
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=60
        )
        elapsed_time = time.time() - start_time
        
        if result.returncode != 0:
            return jsonify({
                'success': False,
                'error': result.stderr
            }), 500
        
        # Read results from CSV files in temp directory
        results = {}
        for strategy in strategies:
            stats_file = temp_results_dir / f'{strategy}_statistics.csv'
            pnl_file = temp_results_dir / f'{strategy}_pnl.csv'
            
            stats = {}
            if stats_file.exists():
                with open(stats_file, 'r') as f:
                    reader = csv.DictReader(f)
                    for row in reader:
                        stats[row['Metric']] = float(row['Value'])
            
            # Get final PnL from PnL file
            final_pnl = 10000.0  # Default
            if pnl_file.exists():
                with open(pnl_file, 'r') as f:
                    reader = csv.DictReader(f)
                    pnl_values = [float(row['PnL']) for row in reader]
                    if pnl_values:
                        final_pnl = pnl_values[-1]
            
            stats['FinalPnL'] = final_pnl
            results[strategy] = {
                'statistics': stats,
                'has_pnl': pnl_file.exists()
            }
        
        # Store temp directory path for this session
        # Files will be served from temp directory, not saved to results/
        import hashlib
        session_key = hashlib.md5(f"{time.time()}{strategies}".encode()).hexdigest()
        app.temp_dirs = getattr(app, 'temp_dirs', {})
        app.temp_dirs[session_key] = temp_dir
        app.strategy_sessions = getattr(app, 'strategy_sessions', {})
        for strategy in strategies:
            app.strategy_sessions[strategy] = session_key
        
        return jsonify({
            'success': True,
            'results': results,
            'elapsed_time': elapsed_time,
            'output': result.stdout,
            'session_key': session_key
        })
        
    except subprocess.TimeoutExpired:
        return jsonify({
            'success': False,
            'error': 'Backtest timeout'
        }), 500
    except Exception as e:
        return jsonify({
            'success': False,
            'error': str(e)
        }), 500

@app.route('/api/results/<strategy>/pnl')
def get_pnl_data(strategy):
    """Get P&L data for charting"""
    try:
        # Get from temp directory (not from results/)
        session_key = app.strategy_sessions.get(strategy)
        if session_key and session_key in app.temp_dirs:
            temp_dir = app.temp_dirs[session_key]
            pnl_file = Path(temp_dir) / 'results' / f'{strategy}_pnl.csv'
        else:
            return jsonify({'error': 'Session not found. Please run backtest first.'}), 404
        
        if not pnl_file.exists():
            return jsonify({'error': 'File not found'}), 404
        
        data = {'index': [], 'pnl': []}
        with open(pnl_file, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                data['index'].append(int(row['Index']))
                data['pnl'].append(float(row['PnL']))
        
        return jsonify(data)
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/results/<strategy>/statistics')
def get_statistics(strategy):
    """Get statistics"""
    try:
        stats_file = RESULTS_DIR / f'{strategy}_statistics.csv'
        if not stats_file.exists():
            return jsonify({'error': 'File not found'}), 404
        
        stats = {}
        with open(stats_file, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                stats[row['Metric']] = float(row['Value'])
        
        return jsonify(stats)
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/download/<strategy>/<file_type>')
def download_file(strategy, file_type):
    """Download CSV file from temp directory"""
    try:
        if file_type == 'pnl':
            filename = f'{strategy}_pnl.csv'
        elif file_type == 'statistics':
            filename = f'{strategy}_statistics.csv'
        else:
            return jsonify({'error': 'Invalid file type'}), 400
        
        # Get file from temp directory (not from results/)
        session_key = app.strategy_sessions.get(strategy)
        if not session_key or session_key not in app.temp_dirs:
            return jsonify({'error': 'Session not found. Please run backtest first.'}), 404
        
        temp_dir = app.temp_dirs[session_key]
        file_path = Path(temp_dir) / 'results' / filename
        
        if not file_path.exists():
            return jsonify({'error': 'File not found. Please run backtest first.'}), 404
        
        return send_file(file_path, as_attachment=True, download_name=filename)
    except Exception as e:
        return jsonify({'error': str(e)}), 500

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=5001)

