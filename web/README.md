# Web Interface

This directory contains the web interface for the Backtest Engine.

## Structure

```
web/
├── app.py              # Flask backend server
├── requirements.txt    # Python dependencies
├── start_web.sh       # Startup script
├── templates/         # HTML templates
│   └── index.html
└── static/           # Static assets
    ├── css/
    │   └── style.css
    └── js/
        └── main.js
```

## Quick Start

1. Install dependencies:
```bash
pip3 install -r requirements.txt
```

2. Start the server:
```bash
./start_web.sh
# or
python3 app.py
```

3. Open browser:
```
http://localhost:5001
```

## Requirements

- Python 3.7+
- Flask
- Docker (for running backtests)

