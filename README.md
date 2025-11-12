# Backtest-Engine

A modular high-performance C++ backtesting engine featuring GBM + Jump tick simulation and multiple plug-and-play trading strategies. Containerized with Docker for easy execution.

---

🚧 **Work in Progress**
This project is actively being developped. Contributions, feedback and ideas are welcome!

---

## ✨ Features
- **GBM + Jump Tick Simulation**
  Simulates realistic tick-level price movements using Geometric Brownian Motion with jumps.
- **Pluggable Strategy Interface**
  Easily add or modify trading strategies (Mean Reversion, Breakout, Spread, etc...)
- **Multi-Strategy Execution**
  Backtest several strategies in parallel on the same tick stream.
- **Web Interface**
  Beautiful web-based UI for running backtests and viewing results with interactive charts.
- **Dockerized**
  Fully containerized, just build and run with Docker in a few seconds.
- **Performance Monitoring**
  Execution time logging to benchmark strategies and engine throughput.

---

## 📈 Strategies Included
- `MeanReversionSimple` - Buys on price drops, sells on price rises (mean reversion)
- `BreakoutStrategy` - Enters positions when price breaks above/below recent highs/lows
- `SpreadStrategy` - Profits from bid-ask spread using limit orders

Each strategy runs on the same tick series with configurable parameters and initial capital.

---

## 🚀 Getting Started

### Requirements
- Docker
- Python 3.7+ (for web interface)
- Git

### Option 1: Web Interface (Recommended)

1. **Build the Docker image** (first time only):
```bash
docker build -t backtest-engine .
```

2. **Start the web interface**:
```bash
cd web
./start_web.sh
```

3. **Open your browser**:
```
http://localhost:5001
```

The web interface allows you to:
- Configure backtest parameters (number of ticks, initial capital)
- Select which strategies to run
- View results with interactive charts
- Download CSV files with statistics and P&L data

### Option 2: Command Line (Docker)

1. **Build the image**:
```bash
docker build -t backtest-engine .
```

2. **Run the backtest**:
```bash
docker run --rm -v "$(pwd)/results:/app/results" -w /app backtest-engine sh -c "mkdir -p results && ./build/BacktestEngine && mv *.csv results/ 2>/dev/null || true"
```

3. **View results**:
Results will be saved in the `results/` directory as CSV files.

---

## 📁 Project Structure

```
Backtest-Engine/
├── Core/              # Core engine components
├── Simulation/        # Market data simulation
├── Strategies/        # Trading strategies
├── web/              # Web interface (Flask)
│   ├── app.py        # Backend server
│   ├── templates/    # HTML templates
│   └── static/       # CSS, JavaScript
├── BacktestEngine_Project.cpp  # Main entry point
├── CMakeLists.txt    # Build configuration
└── Dockerfile        # Docker configuration
```

---

## 🛠️ Development

### Building Locally

```bash
mkdir build && cd build
cmake ..
make
./BacktestEngine
```

### Web Interface Development

```bash
cd web
pip3 install -r requirements.txt
python3 app.py
```

---

## 📊 Results

After running a backtest, you'll get:
- **Statistics CSV**: Key metrics (Sharpe ratio, max drawdown, volatility, etc.)
- **P&L CSV**: Portfolio value over time for charting

---

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

---

## 📄 License

See [LICENSE](LICENSE) file for details.
