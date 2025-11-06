# zohd - Port Conflict Resolver

**zohd** is a fast, cross-platform CLI tool that automatically detects, resolves, and manages port conflicts on developer machines. Written in modern C++17 with zero runtime dependencies.

## Features

- **Zero dependencies** - Single binary, no runtime requirements
- **Cross-platform** - Works on Windows, macOS, and Linux
- **Fast execution** - Most operations complete in < 100ms
- **Intuitive CLI** - Simple commands, no learning curve
- **Safe operations** - Confirms before killing processes

## Installation

### From Source

```bash
# Clone the repository
git clone https://github.com/yourusername/zohd.git
cd zohd

# Build
mkdir build && cd build
cmake ..
cmake --build . --config Release

# Install (optional)
sudo cmake --install .
```

### Requirements

- CMake 3.15+
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)

## Usage

### Scan Common Development Ports

Quickly scan ports commonly used in development (3000-3010, 5000-5010, 8000-8090, 9000-9010):

```bash
zohd scan
```

Output:
```
Scanning common development ports...

✓  3000 - FREE
✗  3001 - USED by node (PID 1234) [2h ago]
✓  5000 - FREE
✗  8080 - USED by docker-proxy (PID 5678)
✓  8081 - FREE

Summary: 2 ports busy, 109 ports free
```

### Check Specific Port

```bash
zohd check 3000
```

Output if in use:
```
Port 3000 is IN USE
  Process: node
  PID: 1234
  Command: node server.js
  User: john
  Started: 2h ago
```

### Kill Process on Port

Interactive mode (prompts for confirmation):
```bash
zohd kill 3000
```

Force mode (immediate kill):
```bash
zohd kill 3000 --force
```

### Suggest Free Ports

```bash
zohd suggest
```

Get more suggestions:
```bash
zohd suggest --count 10
```

Output:
```
Available ports in development ranges:
  3000 - Commonly used for React/Node
  3002
  5001
  8081
  9000
```

### List All Active Ports

```bash
zohd list
```

Output:
```
Active ports:
  PORT   PID    PROCESS          COMMAND                    USER
  ------------------------------------------------------------------------
  3000   1234   node             node server.js             john
  3001   5678   python           python app.py              john
  8080   9012   docker-proxy     docker-proxy               root
  5432   3456   postgres         postgres -D /data          postgres

Total: 4 active ports
```

### Detailed Port Information

```bash
zohd info 8080
```

Output:
```
Port 8080 Information:
  Status: IN USE
  Process: docker-proxy
  PID: 9012
  User: root
  Command: /usr/local/bin/docker-proxy -proto tcp
  Started: 5d ago
```

### Interactive Fix

```bash
zohd fix 3000
```

Output:
```
Port 3000 is IN USE by node (PID 1234)

Choose action:
1. Kill the process
2. Use alternative port (suggest free port)
3. Show detailed process info
4. Cancel

Enter choice (1-4):
```

## Command Reference

| Command | Description |
|---------|-------------|
| `zohd scan` | Scan common development ports |
| `zohd check <port>` | Check if specific port is in use |
| `zohd kill <port> [--force]` | Kill process using port |
| `zohd suggest [--count N]` | Suggest N free ports (default 5) |
| `zohd list` | List all active ports |
| `zohd info <port>` | Show detailed information about port |
| `zohd fix <port>` | Interactive port conflict resolution |

## Development

### Project Structure

```
zohd/
├── CMakeLists.txt
├── README.md
├── src/
│   ├── main.cpp
│   ├── core/              # Core logic
│   ├── platform/          # Platform-specific implementations
│   ├── cli/               # CLI interface
│   └── utils/             # Utility functions
├── include/
│   └── third_party/       # Header-only libraries
└── tests/
```

### Building with Tests

```bash
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON
cmake --build .
ctest
```

### Static Build

```bash
cmake .. -DBUILD_STATIC=ON
cmake --build .
```

## Platform Support

- **Linux** - Full support (tested on Ubuntu 20.04+, Arch Linux)
- **macOS** - Full support (tested on macOS 11+)
- **Windows** - Full support (tested on Windows 10+)

## Common Use Cases

### Starting a Development Server

```bash
# Check if port is available before starting
zohd check 3000 && npm start

# Or automatically kill conflicting process
zohd kill 3000 --force && npm start
```

### Finding Alternative Ports

```bash
# Port 8080 is busy, find alternatives
zohd suggest --count 5
```

### Debugging Port Conflicts

```bash
# See what's using the port
zohd info 3000

# See all active development ports
zohd scan
```

## Troubleshooting

### Permission Denied When Killing Process

If you see "permission denied" errors, you may need elevated privileges to kill processes owned by other users:

```bash
sudo zohd kill 3000 --force
```

### Process Not Found

Some processes may have terminated between the check and kill operations. This is normal and not an error.

## License

MIT License - See LICENSE file for details

## Contributing

Contributions are welcome! Please see CONTRIBUTING.md for guidelines.

## Acknowledgments

- [CLI11](https://github.com/CLIUtils/CLI11) - Command line parsing library
