// Main JavaScript file

let charts = {};

document.getElementById('backtestForm').addEventListener('submit', async (e) => {
    e.preventDefault();
    
    const runBtn = document.getElementById('runBtn');
    const loading = document.getElementById('loading');
    const results = document.getElementById('results');
    const error = document.getElementById('error');
    const errorMessage = document.getElementById('errorMessage');
    
    // Reset state
    error.style.display = 'none';
    results.style.display = 'none';
    loading.style.display = 'block';
    runBtn.disabled = true;
    runBtn.innerHTML = '<i class="bi bi-hourglass-split me-2"></i>Running...';
    
    // Get configuration
    const numTicks = document.getElementById('num_ticks').value;
    const initialCapital = document.getElementById('initial_capital').value;
    const strategies = [];
    if (document.getElementById('strategy_mean').checked) strategies.push('Mean_Reversion');
    if (document.getElementById('strategy_breakout').checked) strategies.push('Breakout_Win20');
    if (document.getElementById('strategy_spread').checked) strategies.push('Spread');
    
    if (strategies.length === 0) {
        errorMessage.textContent = 'Please select at least one strategy';
        error.style.display = 'block';
        loading.style.display = 'none';
        runBtn.disabled = false;
        runBtn.innerHTML = '<i class="bi bi-play-fill me-2"></i>Run Backtest';
        return;
    }
    
    try {
        const response = await fetch('/api/run', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({
                num_ticks: parseInt(numTicks),
                initial_capital: parseFloat(initialCapital),
                strategies: strategies
            })
        });
        
        const data = await response.json();
        
        if (data.success) {
            displayResults(data);
        } else {
            throw new Error(data.error || 'Backtest failed');
        }
    } catch (err) {
        errorMessage.textContent = 'Error: ' + err.message;
        error.style.display = 'block';
    } finally {
        loading.style.display = 'none';
        runBtn.disabled = false;
        runBtn.innerHTML = '<i class="bi bi-play-fill me-2"></i>Run Backtest';
    }
});

function displayResults(data) {
    const resultsDiv = document.getElementById('results');
    const strategyResultsDiv = document.getElementById('strategyResults');
    const elapsedTimeSpan = document.getElementById('elapsedTime');
    
    elapsedTimeSpan.textContent = data.elapsed_time.toFixed(3);
    resultsDiv.style.display = 'block';
    strategyResultsDiv.innerHTML = '';
    
    // Scroll to results
    resultsDiv.scrollIntoView({ behavior: 'smooth', block: 'start' });
    
    // Create cards for each strategy
    for (const [strategy, result] of Object.entries(data.results)) {
        const card = createStrategyCard(strategy, result);
        strategyResultsDiv.appendChild(card);
        
        // Load chart after a short delay to ensure DOM is ready
        if (result.has_pnl) {
            setTimeout(() => loadChart(strategy), 100);
        }
    }
    
    // Initialize Bootstrap tooltips
    var tooltipTriggerList = [].slice.call(document.querySelectorAll('[data-bs-toggle="tooltip"]'));
    var tooltipList = tooltipTriggerList.map(function (tooltipTriggerEl) {
        return new bootstrap.Tooltip(tooltipTriggerEl);
    });
}

function createStrategyCard(strategy, result) {
    const stats = result.statistics;
    const finalPnL = stats.FinalPnL || (stats.TotalReturn ? 10000 * (1 + stats.TotalReturn) : 10000);
    const totalReturn = stats.TotalReturn || 0;
    const sharpe = stats.Sharpe || 0;
    const maxDrawdown = stats.MaxDrawdown || 0;
    const volatility = stats.AnnualizedVolatility || 0;
    
    // Determine header color based on performance
    let headerClass = 'strategy-header-warning';
    if (totalReturn > 0 && sharpe > 1) {
        headerClass = 'strategy-header-success';
    } else if (totalReturn < 0 || sharpe < 0) {
        headerClass = 'strategy-header-danger';
    }
    
    const card = document.createElement('div');
    card.className = 'col-md-4';
    card.innerHTML = `
        <div class="card strategy-card h-100 shadow-sm">
            <div class="card-header ${headerClass} text-white">
                <h5 class="mb-0 d-flex align-items-center">
                    <i class="bi bi-graph-up me-2"></i>
                    ${getStrategyName(strategy)}
                </h5>
            </div>
            <div class="card-body p-4">
                <div class="mb-4">
                    <div class="pnl-value ${totalReturn >= 0 ? 'text-success' : 'text-danger'}">
                        $${finalPnL.toLocaleString('en-US', {minimumFractionDigits: 2, maximumFractionDigits: 2})}
                    </div>
                    <div class="pnl-label">Final P&L</div>
                </div>
                
                <div class="mb-3">
                    <div class="d-flex flex-wrap gap-2">
                        <span class="metric-badge ${totalReturn >= 0 ? 'metric-positive' : 'metric-negative'}">
                            <i class="bi bi-arrow-${totalReturn >= 0 ? 'up' : 'down'}-right me-1"></i>
                            Return: ${(totalReturn * 100).toFixed(2)}%
                        </span>
                        <span class="metric-badge ${sharpe > 1 ? 'metric-positive' : sharpe > 0 ? 'metric-neutral' : 'metric-negative'}">
                            <i class="bi bi-speedometer2 me-1"></i>
                            Sharpe: ${sharpe.toFixed(2)}
                        </span>
                        <span class="metric-badge metric-negative">
                            <i class="bi bi-arrow-down me-1"></i>
                            Max DD: ${(maxDrawdown * 100).toFixed(2)}%
                        </span>
                        ${volatility > 0 ? `
                            <span class="metric-badge metric-neutral">
                                <i class="bi bi-activity me-1"></i>
                                Vol: ${(volatility * 100).toFixed(2)}%
                            </span>
                        ` : ''}
                    </div>
                </div>
                
                ${result.has_pnl ? `
                    <div class="chart-container">
                        <canvas id="chart-${strategy}"></canvas>
                    </div>
                ` : ''}
                
                <div class="mt-4">
                    <div class="d-flex gap-2 mb-2">
                        <a href="/api/download/${strategy}/statistics" class="btn btn-sm btn-outline-primary flex-fill" 
                           data-bs-toggle="tooltip" data-bs-placement="top" 
                           title="Download key performance metrics (Sharpe ratio, max drawdown, volatility, etc.) as CSV">
                            <i class="bi bi-download me-1"></i>Statistics
                        </a>
                        ${result.has_pnl ? `
                            <a href="/api/download/${strategy}/pnl" class="btn btn-sm btn-outline-primary flex-fill"
                               data-bs-toggle="tooltip" data-bs-placement="top"
                               title="Download portfolio value over time for charting and analysis">
                                <i class="bi bi-download me-1"></i>P&L Data
                            </a>
                        ` : ''}
                    </div>
                    <small class="text-muted d-block text-center">
                        <i class="bi bi-info-circle me-1"></i>
                        <strong>Statistics:</strong> Key metrics (Sharpe, drawdown, volatility) | 
                        <strong>P&L Data:</strong> Portfolio value history
                    </small>
                </div>
            </div>
        </div>
    `;
    
    return card;
}

function getStrategyName(strategy) {
    const names = {
        'Mean_Reversion': 'Mean Reversion',
        'Breakout_Win20': 'Breakout Strategy',
        'Spread': 'Spread Strategy'
    };
    return names[strategy] || strategy;
}

async function loadChart(strategy) {
    try {
        const response = await fetch(`/api/results/${strategy}/pnl`);
        const data = await response.json();
        
        const ctx = document.getElementById(`chart-${strategy}`);
        if (!ctx) return;
        
        // Destroy old chart
        if (charts[strategy]) {
            charts[strategy].destroy();
        }
        
        // Determine color based on final PnL
        const finalPnL = data.pnl[data.pnl.length - 1];
        const initialPnL = data.pnl[0];
        const isPositive = finalPnL >= initialPnL;
        
        // Create new chart
        charts[strategy] = new Chart(ctx, {
            type: 'line',
            data: {
                labels: data.index,
                datasets: [{
                    label: 'P&L',
                    data: data.pnl,
                    borderColor: isPositive ? 'rgb(40, 167, 69)' : 'rgb(220, 53, 69)',
                    backgroundColor: isPositive ? 'rgba(40, 167, 69, 0.1)' : 'rgba(220, 53, 69, 0.1)',
                    borderWidth: 2,
                    tension: 0.4,
                    fill: true,
                    pointRadius: 0,
                    pointHoverRadius: 4
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: {
                        display: false
                    },
                    tooltip: {
                        mode: 'index',
                        intersect: false,
                        callbacks: {
                            label: function(context) {
                                return 'P&L: $' + context.parsed.y.toFixed(2);
                            }
                        }
                    }
                },
                scales: {
                    x: {
                        display: true,
                        title: {
                            display: true,
                            text: 'Tick Index'
                        },
                        grid: {
                            display: false
                        }
                    },
                    y: {
                        display: true,
                        title: {
                            display: true,
                            text: 'Portfolio Value ($)'
                        },
                        beginAtZero: false,
                        grid: {
                            color: 'rgba(0, 0, 0, 0.05)'
                        }
                    }
                },
                interaction: {
                    mode: 'nearest',
                    axis: 'x',
                    intersect: false
                }
            }
        });
    } catch (err) {
        console.error('Failed to load chart:', err);
    }
}
