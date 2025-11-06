#!/bin/bash
# Debug script to test port parsing

# Show first few lines of /proc/net/tcp
echo "=== /proc/net/tcp (first 5 LISTEN ports) ==="
awk '$4 == "0A" {print}' /proc/net/tcp | head -5

echo ""
echo "=== Parsing test ==="

# Parse first LISTEN line
line=$(awk '$4 == "0A" {print; exit}' /proc/net/tcp)
echo "Line: $line"

# Extract fields
sl=$(echo "$line" | awk '{print $1}')
local_addr=$(echo "$line" | awk '{print $2}')
rem_addr=$(echo "$line" | awk '{print $3}')
st=$(echo "$line" | awk '{print $4}')

echo "sl: $sl"
echo "local_addr: $local_addr"
echo "rem_addr: $rem_addr"
echo "st: $st"

# Extract port
port_hex=$(echo "$local_addr" | cut -d: -f2)
echo "port_hex: $port_hex"

# Convert to decimal
port_dec=$((16#$port_hex))
echo "port_dec: $port_dec"
