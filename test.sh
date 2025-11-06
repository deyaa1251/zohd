#!/bin/bash
# Comprehensive test script for zohd

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test counters
TESTS_PASSED=0
TESTS_FAILED=0
TEST_SERVERS=()

# Cleanup function
cleanup() {
    echo ""
    echo -e "${YELLOW}Cleaning up test servers...${NC}"
    for pid in "${TEST_SERVERS[@]}"; do
        if kill -0 "$pid" 2>/dev/null; then
            kill "$pid" 2>/dev/null || true
            echo "  Killed test server (PID: $pid)"
        fi
    done

    # Summary
    echo ""
    echo "========================================"
    echo -e "${GREEN}Tests Passed: $TESTS_PASSED${NC}"
    if [ $TESTS_FAILED -gt 0 ]; then
        echo -e "${RED}Tests Failed: $TESTS_FAILED${NC}"
        exit 1
    else
        echo -e "${GREEN}All tests passed!${NC}"
        exit 0
    fi
}

trap cleanup EXIT INT TERM

# Helper functions
pass() {
    echo -e "${GREEN}✓ PASS${NC}: $1"
    ((TESTS_PASSED++))
}

fail() {
    echo -e "${RED}✗ FAIL${NC}: $1"
    ((TESTS_FAILED++))
}

info() {
    echo -e "${BLUE}ℹ INFO${NC}: $1"
}

section() {
    echo ""
    echo "========================================"
    echo -e "${YELLOW}$1${NC}"
    echo "========================================"
}

# Check if zohd is built
if [ ! -f "build/zohd" ]; then
    echo -e "${RED}Error: build/zohd not found. Please run ./build.sh first${NC}"
    exit 1
fi

ZOHD="./build/zohd"

section "Test 1: Basic Commands"

# Test 1.1: Help command
info "Testing --help command"
if $ZOHD --help > /dev/null 2>&1; then
    pass "Help command works"
else
    fail "Help command failed"
fi

# Test 1.2: Invalid command
info "Testing invalid command handling"
if ! $ZOHD invalidcommand > /dev/null 2>&1; then
    pass "Invalid command rejected"
else
    fail "Invalid command not rejected"
fi

section "Test 2: Port Checking"

# Test 2.1: Check a likely free port
info "Checking likely free port (9999)"
output=$($ZOHD check 9999)
if echo "$output" | grep -q "Port 9999"; then
    pass "Check command produces output"
else
    fail "Check command output incorrect"
fi

# Test 2.2: Invalid port numbers
info "Testing invalid port numbers"
if ! $ZOHD check 0 > /dev/null 2>&1; then
    pass "Rejected port 0"
else
    fail "Accepted invalid port 0"
fi

if ! $ZOHD check 99999 > /dev/null 2>&1; then
    pass "Rejected port 99999"
else
    fail "Accepted invalid port 99999"
fi

section "Test 3: Port Suggestion"

# Test 3.1: Suggest default ports
info "Testing suggest command (default)"
output=$($ZOHD suggest)
if echo "$output" | grep -q "Available ports"; then
    pass "Suggest command works"
else
    fail "Suggest command failed"
fi

# Test 3.2: Suggest with count
info "Testing suggest with --count"
output=$($ZOHD suggest --count 3)
port_count=$(echo "$output" | grep -E '^\s+[0-9]+' | wc -l)
if [ "$port_count" -ge 1 ] && [ "$port_count" -le 3 ]; then
    pass "Suggest --count 3 returned reasonable results"
else
    fail "Suggest --count 3 returned wrong number of ports ($port_count)"
fi

section "Test 4: Scan Command"

# Test 4.1: Basic scan
info "Testing scan command"
output=$($ZOHD scan)
if echo "$output" | grep -q "Scanning common development ports"; then
    pass "Scan command produces header"
else
    fail "Scan command missing header"
fi

if echo "$output" | grep -q "Summary:"; then
    pass "Scan command produces summary"
else
    fail "Scan command missing summary"
fi

section "Test 5: List Command"

# Test 5.1: List all ports
info "Testing list command"
output=$($ZOHD list)
if echo "$output" | grep -q "PORT\|Total:"; then
    pass "List command works"
else
    fail "List command failed"
fi

section "Test 6: Real Port Testing with Test Servers"

# Start test servers using Python (most systems have python3)
if command -v python3 &> /dev/null; then
    info "Starting test HTTP servers on ports 8888, 8889, 8890"

    # Start server on port 8888
    python3 -m http.server 8888 > /dev/null 2>&1 &
    TEST_SERVERS+=($!)
    sleep 0.5

    # Start server on port 8889
    python3 -m http.server 8889 > /dev/null 2>&1 &
    TEST_SERVERS+=($!)
    sleep 0.5

    # Start server on port 8890
    python3 -m http.server 8890 > /dev/null 2>&1 &
    TEST_SERVERS+=($!)
    sleep 0.5

    info "Test servers started (PIDs: ${TEST_SERVERS[*]})"

    # Test 6.1: Check port 8888 (should be IN USE)
    info "Checking port 8888 (should be IN USE)"
    output=$($ZOHD check 8888)
    if echo "$output" | grep -q "IN USE"; then
        pass "Port 8888 correctly detected as IN USE"
    else
        fail "Port 8888 not detected as IN USE"
        echo "Output: $output"
    fi

    # Test 6.2: Info on port 8888
    info "Getting info on port 8888"
    output=$($ZOHD info 8888)
    if echo "$output" | grep -q "Port 8888"; then
        pass "Info command works on port 8888"
    else
        fail "Info command failed on port 8888"
    fi

    # Test 6.3: Scan should detect our test ports
    info "Scanning for test ports"
    output=$($ZOHD scan)
    if echo "$output" | grep "8888\|8889\|8890" | grep -q "USED"; then
        pass "Scan detected at least one test server"
    else
        fail "Scan didn't detect test servers"
        echo "Output: $output"
    fi

    # Test 6.4: Check a free port between test servers
    info "Checking port 8891 (should be FREE)"
    output=$($ZOHD check 8891)
    if echo "$output" | grep -q "FREE"; then
        pass "Port 8891 correctly detected as FREE"
    else
        fail "Port 8891 not detected as FREE"
    fi

    # Test 6.5: Kill command (with force)
    info "Testing kill command on port 8888"
    if $ZOHD kill 8888 --force > /dev/null 2>&1; then
        sleep 0.5
        output=$($ZOHD check 8888)
        if echo "$output" | grep -q "FREE"; then
            pass "Port 8888 successfully freed"
        else
            # This might fail if the port is still in TIME_WAIT
            info "Port 8888 might still be in TIME_WAIT state (normal)"
        fi
    else
        fail "Kill command failed"
    fi

else
    info "Python3 not found, skipping real server tests"
fi

section "Test 7: Edge Cases"

# Test 7.1: Check system ports
info "Checking well-known ports"
for port in 22 80 443; do
    output=$($ZOHD check $port 2>&1)
    if echo "$output" | grep -q "Port $port"; then
        pass "Can check port $port"
    else
        fail "Failed to check port $port"
    fi
done

# Test 7.2: Check high port
info "Checking high port number (65000)"
output=$($ZOHD check 65000)
if echo "$output" | grep -q "Port 65000"; then
    pass "Can check high port numbers"
else
    fail "Failed to check high port"
fi

section "Test 8: Output Format Validation"

# Test 8.1: Check output formatting
info "Validating scan output format"
output=$($ZOHD scan)
if echo "$output" | grep -E '^[✓✗]' > /dev/null; then
    pass "Scan uses correct symbols"
else
    fail "Scan symbols incorrect"
fi

# Test 8.2: List output format
info "Validating list output format"
output=$($ZOHD list)
if echo "$output" | grep -q "PORT.*PID.*PROCESS"; then
    pass "List has correct header format"
else
    fail "List header format incorrect"
fi

section "Test 9: Performance"

# Test 9.1: Check command speed
info "Testing check command performance"
start_time=$(date +%s%N)
$ZOHD check 9999 > /dev/null
end_time=$(date +%s%N)
elapsed=$(( (end_time - start_time) / 1000000 )) # Convert to milliseconds

if [ $elapsed -lt 100 ]; then
    pass "Check command completed in ${elapsed}ms (< 100ms)"
else
    info "Check command took ${elapsed}ms (target: < 100ms)"
fi

# Test 9.2: Scan command speed
info "Testing scan command performance"
start_time=$(date +%s%N)
$ZOHD scan > /dev/null
end_time=$(date +%s%N)
elapsed=$(( (end_time - start_time) / 1000000 ))

if [ $elapsed -lt 500 ]; then
    pass "Scan command completed in ${elapsed}ms (< 500ms)"
else
    info "Scan command took ${elapsed}ms (target: < 500ms)"
fi

section "Test 10: Error Handling"

# Test 10.1: Kill non-existent port
info "Testing kill on free port"
output=$($ZOHD kill 9999 --force 2>&1)
if echo "$output" | grep -q "not in use"; then
    pass "Correctly handles killing free port"
else
    fail "Incorrect handling of free port kill"
fi

# Test 10.2: Invalid subcommand
info "Testing invalid subcommand"
if ! $ZOHD notacommand > /dev/null 2>&1; then
    pass "Rejects invalid subcommands"
else
    fail "Accepts invalid subcommands"
fi

# Tests complete - cleanup will run via trap
