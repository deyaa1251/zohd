#!/bin/bash
# Installation script for zohd

set -e

echo "Installing zohd..."

# Build if not already built
if [ ! -f "build/zohd" ]; then
    echo "Building zohd..."
    ./build.sh
fi

# Install to /usr/local/bin (requires sudo)
echo "Installing to /usr/local/bin (requires sudo)..."
sudo cp build/zohd /usr/local/bin/zohd
sudo chmod +x /usr/local/bin/zohd

echo ""
echo "Installation complete!"
echo "Run 'zohd --help' to get started"
