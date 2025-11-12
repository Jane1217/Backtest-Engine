#!/usr/bin/env python3
"""
Backtest Engine Web Interface
简单的 Flask 后端，提供 Web API
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
    """主页面"""
    return render_template('index.html')

@app.route('/api/run', methods=['POST'])
def run_backtest():
    """运行回测"""
    try:
        data = request.json
        num_ticks = int(data.get('num_ticks', 1000))
        initial_capital = float(data.get('initial_capital', 10000))
        strategies = data.get('strategies', ['Mean_Reversion', 'Breakout_Win20', 'Spread'])
        
        # 确保结果目录存在
        RESULTS_DIR.mkdir(exist_ok=True)
        
        # 运行 Docker 容器执行回测
        cmd = [
            'docker', 'run', '--rm',
            '-v', f'{RESULTS_DIR.absolute()}:/app/results',
            '-w', '/app',
            'backtest-engine',
            'sh', '-c',
            f'mkdir -p results && ./build/BacktestEngine && mv *.csv results/ 2>/dev/null || true'
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
        
        # 读取结果
        results = {}
        for strategy in strategies:
            stats_file = RESULTS_DIR / f'{strategy}_statistics.csv'
            pnl_file = RESULTS_DIR / f'{strategy}_pnl.csv'
            
            stats = {}
            if stats_file.exists():
                with open(stats_file, 'r') as f:
                    reader = csv.DictReader(f)
                    for row in reader:
                        stats[row['Metric']] = float(row['Value'])
            
            # 从 PnL 文件获取最终盈亏
            final_pnl = 10000.0  # 默认值
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
        
        return jsonify({
            'success': True,
            'results': results,
            'elapsed_time': elapsed_time,
            'output': result.stdout
        })
        
    except subprocess.TimeoutExpired:
        return jsonify({
            'success': False,
            'error': '回测超时'
        }), 500
    except Exception as e:
        return jsonify({
            'success': False,
            'error': str(e)
        }), 500

@app.route('/api/results/<strategy>/pnl')
def get_pnl_data(strategy):
    """获取盈亏数据（用于图表）"""
    try:
        pnl_file = RESULTS_DIR / f'{strategy}_pnl.csv'
        if not pnl_file.exists():
            return jsonify({'error': '文件不存在'}), 404
        
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
    """获取统计指标"""
    try:
        stats_file = RESULTS_DIR / f'{strategy}_statistics.csv'
        if not stats_file.exists():
            return jsonify({'error': '文件不存在'}), 404
        
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
    """下载 CSV 文件"""
    try:
        if file_type == 'pnl':
            filename = f'{strategy}_pnl.csv'
        elif file_type == 'statistics':
            filename = f'{strategy}_statistics.csv'
        else:
            return jsonify({'error': '无效的文件类型'}), 400
        
        file_path = RESULTS_DIR / filename
        if not file_path.exists():
            return jsonify({'error': '文件不存在'}), 404
        
        return send_file(file_path, as_attachment=True, download_name=filename)
    except Exception as e:
        return jsonify({'error': str(e)}), 500

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=5001)

