#!/bin/bash

# Web Interface Startup Script

echo "=== Starting Backtest Engine Web Interface ==="
echo ""

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Check Python
if ! command -v python3 &> /dev/null; then
    echo "Error: Python3 not found"
    exit 1
fi

# Check dependencies
echo "Checking dependencies..."
if ! python3 -c "import flask" 2>/dev/null; then
    echo "Installing Flask..."
    pip3 install -r requirements.txt
fi

# Check Docker
if ! docker ps > /dev/null 2>&1; then
    echo "Warning: Docker is not running, please start Docker Desktop"
    exit 1
fi

# Check image (from project root)
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_ROOT"
if ! docker images | grep -q backtest-engine; then
    echo "Building Docker image..."
    docker build -t backtest-engine .
fi

cd "$SCRIPT_DIR"
echo ""
echo "Starting Web server..."
echo "Access at: http://localhost:5001"
echo "Press Ctrl+C to stop the server"
echo ""

python3 app.py

