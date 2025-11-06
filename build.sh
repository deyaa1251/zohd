#!/bin/bash
# Simple build script for zohd (when cmake is not available)

set -e

echo "Building zohd..."

# Create build directory
mkdir -p build
cd build

# Compile all source files
g++ -std=c++17 -Wall -Wextra -O2 -DPLATFORM_LINUX \
    -I../include/third_party \
    -I../src \
    ../src/main.cpp \
    ../src/core/port_info.cpp \
    ../src/core/port_scanner.cpp \
    ../src/core/process_manager.cpp \
    ../src/cli/output_formatter.cpp \
    ../src/platform/linux_impl.cpp \
    -o zohd

echo "Build successful! Binary: build/zohd"
echo ""
echo "Run './build/zohd --help' to get started"
