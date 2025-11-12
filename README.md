# Backtest-Engine

A modular high-performance C++ backtesting engine featuring GBM + Jump tick simulation and multiple plug-and-play trading strategies. Containerized with Docker for easy execution.

---

🚧 **Work in Progress**
This project is actively being developped. Contributions, feedback and ideas are welcome!

---

## ✨ Features
- **GBM + Jump Tick Simulation** - Realistic tick-level price movements
- **Pluggable Strategy Interface** - Easily add or modify trading strategies
- **Multi-Strategy Execution** - Backtest several strategies in parallel
- **Web Interface** - Beautiful web-based UI with interactive charts
- **Dockerized** - Fully containerized for easy execution
- **Performance Monitoring** - Execution time logging

---

## 📈 Strategies Included
- `MeanReversionSimple` - Buys on price drops, sells on price rises
- `BreakoutStrategy` - Enters positions on price breakouts
- `SpreadStrategy` - Profits from bid-ask spread

---

## 🚀 Getting Started

### Requirements
- Docker
- Python 3.7+ (for web interface)

### Option 1: Web Interface (Recommended)

1. **Build the Docker image**:
```bash
docker build -t backtest-engine .
```

2. **Start the web interface**:
```bash
cd web
./start_web.sh
```

3. **Open browser**: `http://localhost:5001`

The web interface allows you to configure parameters, view results with charts, and download CSV files.

### Option 2: Command Line

```bash
docker build -t backtest-engine .
docker run --rm backtest-engine
```

---

## 📁 Project Structure

```
Backtest-Engine/
├── Core/              # Core engine components
├── Simulation/        # Market data simulation
├── Strategies/        # Trading strategies
├── web/              # Web interface
└── BacktestEngine_Project.cpp  # Main entry point
```

---

## 📊 Results

When using the web interface:
- Results are displayed with interactive charts
- CSV files (Statistics and P&L Data) are available for download
- Files are only generated when you click the download button

---

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

---

## 📄 License

See [LICENSE](LICENSE) file for details.
