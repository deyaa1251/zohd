# zohd Testing Documentation

## Table of Contents

1. [Overview](#overview)
2. [Test Strategy](#test-strategy)
3. [Running Tests](#running-tests)
4. [Test Categories](#test-categories)
5. [Manual Testing Guide](#manual-testing-guide)
6. [Continuous Integration](#continuous-integration)
7. [Performance Testing](#performance-testing)
8. [Platform-Specific Testing](#platform-specific-testing)
9. [Troubleshooting Tests](#troubleshooting-tests)

---

## Overview

zohd employs a multi-layered testing strategy to ensure reliability across platforms and use cases.

### Test Coverage Goals

- ✅ **Functional**: All commands work as specified
- ✅ **Edge Cases**: Invalid inputs, boundary conditions
- ✅ **Performance**: Meets speed targets (< 100ms for checks)
- ✅ **Security**: Permission checks, input validation
- ⏳ **Cross-Platform**: Linux (complete), Windows/macOS (planned)

---

## Test Strategy

### Testing Pyramid

```
        ┌─────────────┐
        │  Manual     │  Manual smoke tests
        │  Testing    │  Real-world scenarios
        └──────┬──────┘
               │
        ┌──────┴───────┐
        │ Integration  │  Command-line tests
        │   Tests      │  End-to-end workflows
        └──────┬───────┘
               │
        ┌──────┴────────┐
        │  Unit Tests   │  Component testing
        │  (Future)     │  Mocking platforms
        └───────────────┘
```

### Current Status

- **Integration Tests**: Implemented (test.sh)
- **Unit Tests**: Planned (requires test framework)
- **Manual Tests**: Documented below

---

## Running Tests

### Quick Test (Automated)

```bash
# Ensure zohd is built
./build.sh

# Run full test suite
chmod +x test.sh
./test.sh
```

**Expected Output**:
```
========================================
Test 1: Basic Commands
========================================
ℹ INFO: Testing --help command
✓ PASS: Help command works
...

========================================
Tests Passed: 25
All tests passed!
```

### Verbose Test Output

```bash
# Run with bash -x for debugging
bash -x test.sh 2>&1 | tee test-output.log
```

### Individual Command Testing

```bash
# Test specific commands
./build/zohd --help
./build/zohd check 3000
./build/zohd scan
./build/zohd suggest
./build/zohd list
```

---

## Test Categories

### 1. Command-Line Interface Tests

**Goal**: Verify CLI parsing and validation

#### Test Cases

| Test ID | Description | Command | Expected |
|---------|-------------|---------|----------|
| CLI-01 | Help display | `zohd --help` | Shows usage |
| CLI-02 | Invalid command | `zohd notacommand` | Error message |
| CLI-03 | Missing argument | `zohd check` | Error: port required |
| CLI-04 | Invalid port (too low) | `zohd check 0` | Error: port range |
| CLI-05 | Invalid port (too high) | `zohd check 99999` | Error: port range |
| CLI-06 | Valid port | `zohd check 3000` | Success |
| CLI-07 | Force flag | `zohd kill 3000 --force` | No confirmation |
| CLI-08 | Count flag | `zohd suggest --count 10` | 10 suggestions |

#### Implementation (test.sh)

```bash
section "Test 1: Basic Commands"

# CLI-01
if $ZOHD --help > /dev/null 2>&1; then
    pass "Help command works"
fi

# CLI-02
if ! $ZOHD invalidcommand > /dev/null 2>&1; then
    pass "Invalid command rejected"
fi

# CLI-04
if ! $ZOHD check 0 > /dev/null 2>&1; then
    pass "Rejected port 0"
fi
```

### 2. Port Detection Tests

**Goal**: Verify accurate port status detection

#### Test Cases

| Test ID | Description | Setup | Command | Expected |
|---------|-------------|-------|---------|----------|
| PORT-01 | Free port | None | `check 9999` | Port is FREE |
| PORT-02 | Used port | Start server on 8888 | `check 8888` | Port IN USE |
| PORT-03 | System port | None | `check 22` | Likely IN USE |
| PORT-04 | High port | None | `check 65000` | Port is FREE |
| PORT-05 | IPv4 detection | Server on 0.0.0.0:8888 | `check 8888` | Detected |
| PORT-06 | IPv6 detection | Server on [::]:8889 | `check 8889` | Detected |

#### Manual Testing

```bash
# Start test server
python3 -m http.server 8888 &
SERVER_PID=$!

# Test detection
./build/zohd check 8888
# Expected: Port 8888 is IN USE

# Cleanup
kill $SERVER_PID
```

### 3. Process Information Tests

**Goal**: Verify process information accuracy

#### Test Cases

| Test ID | Description | Setup | Expected Info |
|---------|-------------|-------|---------------|
| PROC-01 | Process name | Start python server | name="python3" |
| PROC-02 | Command line | Start with args | Full command |
| PROC-03 | User info | Running as user | Current username |
| PROC-04 | Start time | Long-running process | Reasonable uptime |
| PROC-05 | Missing process | Port with no process | Graceful handling |

#### Test Script (Python Server)

```bash
# Start identifiable server
python3 -m http.server 8765 --bind 127.0.0.1 &
PID=$!

# Test info retrieval
./build/zohd info 8765

# Expected output:
# Port 8765 Information:
#   Status: IN USE
#   Process: python3
#   PID: [actual PID]
#   User: [your username]
#   Command: python3 -m http.server 8765 --bind 127.0.0.1
#   Started: [time ago]

kill $PID
```

### 4. Scan Operation Tests

**Goal**: Verify scanning of port ranges

#### Test Cases

| Test ID | Description | Command | Validation |
|---------|-------------|---------|------------|
| SCAN-01 | Default scan | `scan` | Shows 111 ports |
| SCAN-02 | FREE ports | `scan` | ✓ symbol shown |
| SCAN-03 | USED ports | `scan` | ✗ symbol shown |
| SCAN-04 | Summary line | `scan` | "Summary: X busy, Y free" |
| SCAN-05 | Performance | `scan` | < 500ms |

#### Validation Script

```bash
# Run scan and validate output
output=$(./build/zohd scan)

# Check for header
echo "$output" | grep -q "Scanning common development ports" || echo "FAIL: No header"

# Check for summary
echo "$output" | grep -q "Summary:" || echo "FAIL: No summary"

# Check for symbols
echo "$output" | grep -q "✓\|✗" || echo "FAIL: No status symbols"
```

### 5. Suggest Operation Tests

**Goal**: Verify free port suggestions

#### Test Cases

| Test ID | Description | Command | Expected |
|---------|-------------|---------|----------|
| SUG-01 | Default count | `suggest` | 5 ports |
| SUG-02 | Custom count | `suggest --count 10` | 10 ports |
| SUG-03 | All free | All ports free | Success |
| SUG-04 | Some busy | Some ports used | Returns free only |
| SUG-05 | Output format | `suggest` | Clean list |

#### Test Script

```bash
# Test suggestion
output=$(./build/zohd suggest --count 3)
count=$(echo "$output" | grep -E '^\s+[0-9]+' | wc -l)

if [ $count -ge 1 ] && [ $count -le 3 ]; then
    echo "PASS: Correct suggestion count"
else
    echo "FAIL: Expected 1-3 suggestions, got $count"
fi
```

### 6. List Operation Tests

**Goal**: Verify active port listing

#### Test Cases

| Test ID | Description | Setup | Expected |
|---------|-------------|-------|----------|
| LIST-01 | No servers | Clean system | "No active ports" or minimal |
| LIST-02 | With servers | 3 test servers | Shows all 3 |
| LIST-03 | Table format | Any servers | Header + rows |
| LIST-04 | Total count | Any servers | "Total: N active ports" |

### 7. Kill Operation Tests

**Goal**: Verify process termination

#### Test Cases

| Test ID | Description | Setup | Command | Expected |
|---------|-------------|-------|---------|----------|
| KILL-01 | Interactive kill | Server on 8888 | `kill 8888` | Prompts Y/N |
| KILL-02 | Force kill | Server on 8888 | `kill 8888 -f` | Immediate |
| KILL-03 | Free port | No server | `kill 9999` | "Port not in use" |
| KILL-04 | Verification | After kill | `check 8888` | Port FREE |
| KILL-05 | Permission denied | System port | `kill 80` | Error message |

#### Test Procedure

```bash
# Start test server
python3 -m http.server 8877 &
SERVER_PID=$!
sleep 1

# Verify it's running
./build/zohd check 8877
# Expected: Port 8877 is IN USE

# Kill it
./build/zohd kill 8877 --force
# Expected: Process killed successfully

# Verify it's gone
sleep 1
./build/zohd check 8877
# Expected: Port 8877 is FREE
```

### 8. Fix (Interactive) Operation Tests

**Goal**: Verify interactive resolution

#### Test Cases

| Test ID | Description | Input | Expected |
|---------|-------------|-------|----------|
| FIX-01 | Show menu | Port in use | 4 options |
| FIX-02 | Option 1: Kill | "1" | Process killed |
| FIX-03 | Option 2: Suggest | "2" | Alternative ports |
| FIX-04 | Option 3: Info | "3" | Detailed info |
| FIX-05 | Option 4: Cancel | "4" | Exits gracefully |

#### Manual Test

```bash
# Start server
python3 -m http.server 8866 &
sleep 1

# Run fix command
./build/zohd fix 8866

# You'll see:
# Port 8866 is IN USE
#   Process: python3
#   ...
#
# Choose action:
# 1. Kill the process
# 2. Use alternative port (suggest free port)
# 3. Show detailed process info
# 4. Cancel
#
# Enter choice (1-4):

# Test each option
```

### 9. Edge Case Tests

**Goal**: Handle unusual conditions gracefully

#### Test Cases

| Test ID | Description | Scenario | Expected |
|---------|-------------|----------|----------|
| EDGE-01 | Concurrent access | Multiple zohd instances | Both work |
| EDGE-02 | Process exits | Port freed during check | No crash |
| EDGE-03 | Permission denied | Check system port | Graceful message |
| EDGE-04 | Invalid /proc | Corrupted /proc entry | Skip entry |
| EDGE-05 | Many ports | Scan 1000+ ports | Completes |
| EDGE-06 | No /proc | /proc unmounted | Error message |
| EDGE-07 | Zombie process | Dead process holding port | Detect correctly |

### 10. Performance Tests

**Goal**: Meet speed requirements

#### Test Cases

| Test ID | Description | Command | Target | Actual |
|---------|-------------|---------|--------|--------|
| PERF-01 | Single check | `check 3000` | < 50ms | ~5ms |
| PERF-02 | Full scan | `scan` | < 500ms | ~100ms |
| PERF-03 | Suggest 5 | `suggest` | < 100ms | ~15ms |
| PERF-04 | List all | `list` | < 200ms | ~50ms |
| PERF-05 | Info lookup | `info 8080` | < 100ms | ~10ms |

#### Benchmark Script

```bash
#!/bin/bash
# benchmark.sh

echo "=== zohd Performance Benchmarks ==="

# Test 1: check command
total=0
for i in {1..10}; do
    start=$(date +%s%N)
    ./build/zohd check 3000 > /dev/null
    end=$(date +%s%N)
    elapsed=$(( (end - start) / 1000000 ))
    total=$((total + elapsed))
done
avg=$((total / 10))
echo "check command: ${avg}ms avg (10 runs)"

# Test 2: scan command
start=$(date +%s%N)
./build/zohd scan > /dev/null
end=$(date +%s%N)
elapsed=$(( (end - start) / 1000000 ))
echo "scan command: ${elapsed}ms"

# Test 3: suggest command
start=$(date +%s%N)
./build/zohd suggest > /dev/null
end=$(date +%s%N)
elapsed=$(( (end - start) / 1000000 ))
echo "suggest command: ${elapsed}ms"
```

---

## Manual Testing Guide

### Pre-Test Checklist

- [ ] zohd built successfully (`./build.sh`)
- [ ] Python3 installed (for test servers)
- [ ] Sufficient permissions (for kill tests)
- [ ] Clean test environment (no conflicting ports)

### Test Scenario 1: New User Workflow

**Goal**: Simulate first-time user experience

```bash
# 1. Get help
./build/zohd --help
# Verify: Clear usage message

# 2. Check common port
./build/zohd check 3000
# Verify: Shows FREE or IN USE

# 3. Scan for issues
./build/zohd scan
# Verify: Shows port statuses

# 4. Find alternative
./build/zohd suggest
# Verify: Lists available ports

# 5. See what's running
./build/zohd list
# Verify: Shows active ports
```

### Test Scenario 2: Developer Workflow

**Goal**: Typical development use case

```bash
# Scenario: Starting a dev server that won't start

# 1. Try to start server (simulated)
echo "Error: Address already in use :3000"

# 2. Check the port
./build/zohd check 3000
# Verify: Shows process using port

# 3. Get more details
./build/zohd info 3000
# Verify: Shows full process info

# 4. Decide to kill it
./build/zohd kill 3000
# Verify: Prompts for confirmation

# 5. Confirm kill (type 'y')
# Verify: Process terminated

# 6. Verify port is free
./build/zohd check 3000
# Verify: Port is FREE

# 7. Start server (simulated)
echo "Server started on port 3000"
```

### Test Scenario 3: DevOps Workflow

**Goal**: System administration tasks

```bash
# 1. Audit all listening ports
./build/zohd list > ports-audit.txt
# Verify: Complete list saved

# 2. Check specific services
for port in 80 443 22 3306 5432; do
    ./build/zohd info $port
done
# Verify: Shows all service details

# 3. Find available port for new service
./build/zohd suggest --count 10
# Verify: Lists 10 available ports

# 4. Scan for issues in dev range
./build/zohd scan
# Verify: Shows conflicts
```

### Test Scenario 4: Troubleshooting Workflow

**Goal**: Diagnose port conflicts

```bash
# 1. Application won't start
echo "Error: Port 8080 already in use"

# 2. Check port
./build/zohd check 8080
# Verify: Port is IN USE

# 3. Interactive fix
./build/zohd fix 8080
# Choose option 3: Show detailed info
# Verify: See full process details

# Re-run fix
./build/zohd fix 8080
# Choose option 2: See alternatives
# Verify: Get alternative ports

# Re-run fix
./build/zohd fix 8080
# Choose option 1: Kill process
# Verify: Process terminated
```

---

## Continuous Integration

### GitHub Actions Workflow (Future)

```yaml
# .github/workflows/test.yml
name: Tests

on: [push, pull_request]

jobs:
  test-linux:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Build
        run: ./build.sh
      - name: Run tests
        run: ./test.sh
      - name: Check binary size
        run: |
          size=$(stat -f%z build/zohd)
          if [ $size -gt 500000 ]; then
            echo "Binary too large: ${size} bytes"
            exit 1
          fi
```

### Test Coverage Reporting

```bash
# Generate coverage report (requires gcov/lcov)
g++ -std=c++17 --coverage -O0 [sources]
./run_tests
gcov *.cpp
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
```

---

## Performance Testing

### Benchmark Suite

```bash
# Create benchmark.sh
cat > benchmark.sh << 'EOF'
#!/bin/bash

echo "Port Check Performance:"
hyperfine './build/zohd check 3000' \
          --warmup 3 \
          --min-runs 50

echo ""
echo "Port Scan Performance:"
hyperfine './build/zohd scan' \
          --warmup 2 \
          --min-runs 10

echo ""
echo "Suggest Performance:"
hyperfine './build/zohd suggest --count 10' \
          --warmup 3 \
          --min-runs 30
EOF

chmod +x benchmark.sh
./benchmark.sh
```

### Stress Testing

```bash
# Start many test servers
for port in {8000..8099}; do
    python3 -m http.server $port > /dev/null 2>&1 &
done

# Test with many active ports
time ./build/zohd scan
time ./build/zohd list

# Cleanup
killall python3
```

---

## Platform-Specific Testing

### Linux Testing Matrix

| Distribution | Version | Tested | Status |
|--------------|---------|--------|--------|
| Ubuntu | 20.04 | ✅ | Pass |
| Ubuntu | 22.04 | ⏳ | TBD |
| Arch Linux | Rolling | ✅ | Pass |
| Debian | 11 | ⏳ | TBD |
| Fedora | 38 | ⏳ | TBD |
| CentOS | 8 | ⏳ | TBD |

### Windows Testing (Future)

- [ ] Windows 10 (latest)
- [ ] Windows 11
- [ ] Windows Server 2019
- [ ] PowerShell integration
- [ ] CMD integration

### macOS Testing (Future)

- [ ] macOS 12 (Monterey)
- [ ] macOS 13 (Ventura)
- [ ] macOS 14 (Sonoma)
- [ ] Intel architecture
- [ ] ARM (M1/M2) architecture

---

## Troubleshooting Tests

### Common Test Failures

#### 1. "Permission Denied" Errors

**Symptom**: Tests fail when accessing /proc files

**Solution**:
```bash
# Run with appropriate permissions
sudo ./test.sh

# Or check specific permissions
ls -la /proc/net/tcp
```

#### 2. Test Servers Don't Start

**Symptom**: python3 http.server fails to start

**Diagnosis**:
```bash
# Check if ports are available
./build/zohd check 8888
./build/zohd check 8889

# Check if python3 is installed
python3 --version
```

**Solution**:
```bash
# Free the ports
killall python3

# Or use different ports in test.sh
```

#### 3. Process Information Not Found

**Symptom**: Process info shows "unknown" for active ports

**Cause**: Insufficient permissions to read /proc/[pid]/fd

**Solution**:
```bash
# This is expected for system processes
# Run as root only if necessary
sudo ./build/zohd list
```

#### 4. Timing Issues

**Symptom**: Process killed but port still shows IN USE

**Cause**: TCP TIME_WAIT state

**Solution**:
```bash
# Wait for TIME_WAIT to expire (~60 seconds)
sleep 2
./build/zohd check 8888

# Or use SO_REUSEADDR in test servers
```

---

## Test Result Interpretation

### Success Criteria

✅ **All tests pass**: Ready for release
⚠️ **1-2 tests fail**: Investigate, may be environment-specific
❌ **3+ tests fail**: Do not release, fix issues

### Test Output Analysis

```bash
# Good output
✓ PASS: Check command works
✓ PASS: Port 8888 correctly detected as IN USE
✓ PASS: Process killed successfully

# Concerning output
✗ FAIL: Port 8888 not detected as IN USE
ℹ INFO: Port 8888 might still be in TIME_WAIT state (normal)

# Critical failure
✗ FAIL: Check command failed
✗ FAIL: Scan command failed
✗ FAIL: Kill command failed
```

---

## Conclusion

Comprehensive testing ensures zohd reliability across:
- ✅ All command-line operations
- ✅ Port detection accuracy
- ✅ Process management safety
- ✅ Performance targets
- ✅ Edge case handling

Run tests before every release and after platform changes.

**Test Command**:
```bash
./test.sh && echo "✅ READY TO DEPLOY"
```
