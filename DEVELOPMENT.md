# zohd Development Guide

## Table of Contents

1. [Getting Started](#getting-started)
2. [Development Environment](#development-environment)
3. [Building from Source](#building-from-source)
4. [Code Style Guide](#code-style-guide)
5. [Adding Features](#adding-features)
6. [Platform Implementation](#platform-implementation)
7. [Debugging](#debugging)
8. [Contributing](#contributing)
9. [Release Process](#release-process)

---

## Getting Started

### Prerequisites

**Required**:
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- Git

**Optional**:
- CMake 3.15+ (alternative build system)
- GDB or LLDB (debugging)
- Valgrind (memory leak detection)
- clang-format (code formatting)

### Quick Start

```bash
# Clone the repository
git clone https://github.com/yourusername/zohd.git
cd zohd

# Build
./build.sh

# Test
./build/zohd --help

# Run tests
./test.sh
```

---

## Development Environment

### Recommended IDE Setup

#### VS Code

**Extensions**:
- C/C++ (Microsoft)
- C/C++ Extension Pack
- CMake Tools
- clangd (optional, better than C/C++ IntelliSense)

**Settings** (`.vscode/settings.json`):
```json
{
  "C_Cpp.default.cppStandard": "c++17",
  "C_Cpp.default.compilerPath": "/usr/bin/g++",
  "C_Cpp.default.includePath": [
    "${workspaceFolder}/src",
    "${workspaceFolder}/include/third_party"
  ],
  "files.associations": {
    "*.hpp": "cpp",
    "*.cpp": "cpp"
  }
}
```

**Tasks** (`.vscode/tasks.json`):
```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "build",
      "type": "shell",
      "command": "./build.sh",
      "group": {
        "kind": "build",
        "isDefault": true
      }
    },
    {
      "label": "test",
      "type": "shell",
      "command": "./test.sh",
      "group": "test"
    }
  ]
}
```

#### CLion

**CMakeLists.txt** is provided for CMake-based IDEs.

```bash
# Open project
clion zohd/

# Configure CMake
# Build -> Build Project
# Run -> Run 'zohd'
```

### Editor Configuration

**EditorConfig** (`.editorconfig`):
```ini
root = true

[*]
charset = utf-8
end_of_line = lf
insert_final_newline = true
trim_trailing_whitespace = true

[*.{cpp,hpp}]
indent_style = space
indent_size = 4

[*.{md,yml,yaml}]
indent_style = space
indent_size = 2
```

---

## Building from Source

### Simple Build (No CMake)

```bash
# Build script handles everything
./build.sh

# Output: build/zohd
```

### CMake Build

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
cmake --build . --config Release

# Install (optional)
sudo cmake --install .
```

### Build Options

**Debug Build**:
```bash
g++ -std=c++17 -g -O0 -DDEBUG -DPLATFORM_LINUX \
    -I../include/third_party -I../src \
    [source files] \
    -o zohd-debug
```

**Release Build** (optimized):
```bash
g++ -std=c++17 -O3 -DNDEBUG -DPLATFORM_LINUX \
    -I../include/third_party -I../src \
    [source files] \
    -o zohd

strip zohd  # Remove debug symbols
```

**Static Build** (single binary):
```bash
g++ -std=c++17 -O2 -DPLATFORM_LINUX \
    -static-libgcc -static-libstdc++ \
    -I../include/third_party -I../src \
    [source files] \
    -o zohd-static
```

### Build Variants

| Variant | Flags | Use Case |
|---------|-------|----------|
| Debug | `-g -O0` | Development, debugging |
| Release | `-O3 -DNDEBUG` | Production |
| Profile | `-O2 -pg` | Performance profiling |
| Coverage | `--coverage` | Test coverage analysis |
| Sanitize | `-fsanitize=address` | Memory error detection |

---

## Code Style Guide

### General Principles

1. **Follow existing style** - Consistency matters
2. **Clear over clever** - Readable code > terse code
3. **Comment the why, not the what**
4. **RAII everywhere** - No manual memory management

### Naming Conventions

```cpp
// Classes: PascalCase
class PortScanner { };

// Functions: snake_case
void check_port(uint16_t port);

// Variables: snake_case
int port_number = 3000;
std::string user_name = "john";

// Constants: UPPER_SNAKE_CASE
constexpr int MAX_PORT = 65535;

// Namespaces: lowercase
namespace zohd { }
namespace platform { }

// Enums: PascalCase (type), UPPER_SNAKE_CASE (values)
enum class PortStatus {
    FREE,
    IN_USE,
    UNKNOWN
};
```

### File Organization

```cpp
// header.hpp
#pragma once  // Preferred over include guards

#include <system_headers>
#include "project_headers.hpp"

namespace zohd {

class MyClass {
public:
    // Public interface first
    MyClass();
    void public_method();

private:
    // Private implementation last
    void private_method();
    int member_variable_;
};

} // namespace zohd
```

### Code Formatting

**Indentation**: 4 spaces (no tabs)

**Braces**: Allman style (opening brace on new line for functions)
```cpp
void function_name()
{
    if (condition) {
        // K&R for control structures
    }
}
```

**Line Length**: 100 characters (soft limit)

**Spacing**:
```cpp
// Good
int x = 5;
func(a, b, c);
if (x == 5) {

// Bad
int x=5;
func(a,b,c);
if(x==5){
```

### Comments

```cpp
// Single-line comments for brief explanations
int port = 3000;  // Default development port

/**
 * Multi-line comments for functions/classes
 *
 * @param port The port number to check
 * @return PortInfo structure with status
 */
PortInfo check_port(uint16_t port);

// TODO: Add IPv6 support
// FIXME: Handle permission errors better
// NOTE: This is platform-specific
```

### Error Handling

```cpp
// Prefer exceptions for exceptional cases
if (!file.is_open()) {
    throw std::runtime_error("Cannot open file");
}

// Use return codes for expected failures
bool terminate_process(uint32_t pid) {
    if (kill(pid, SIGTERM) != 0) {
        return false;  // Expected: might not have permission
    }
    return true;
}

// Use std::optional for "not found" cases
std::optional<ProcessInfo> find_process(uint32_t pid) {
    if (!exists(pid)) {
        return std::nullopt;
    }
    return get_info(pid);
}
```

### Modern C++ Practices

```cpp
// Use auto for type inference (when clear)
auto port_info = scanner.check_port(3000);  // Clear from context

// Range-based for loops
for (const auto& port : ports) {
    check_port(port);
}

// Use nullptr instead of NULL
PortInfo* ptr = nullptr;

// Use std::string instead of char*
std::string name = "zohd";  // Not: char name[] = "zohd";

// RAII for resources
{
    std::ifstream file("path");
    // File automatically closed at end of scope
}

// Use smart pointers (when ownership matters)
std::unique_ptr<Resource> resource = std::make_unique<Resource>();
```

---

## Adding Features

### Feature Development Workflow

1. **Plan**: Design the feature (update ARCHITECTURE.md if major)
2. **Branch**: Create feature branch
3. **Implement**: Write code following style guide
4. **Test**: Add tests to test.sh
5. **Document**: Update README.md
6. **Review**: Self-review for quality
7. **Submit**: Create pull request

### Example: Adding a New Command

Let's add a `watch` command that monitors a port.

#### Step 1: Update CLI (main.cpp)

```cpp
// Add to main.cpp
auto* watch_cmd = app.add_subcommand("watch", "Monitor port status");
int watch_port = 0;
int watch_interval = 1;
watch_cmd->add_option("port", watch_port, "Port to monitor")
         ->required()
         ->check(CLI::Range(1, 65535));
watch_cmd->add_option("-i,--interval", watch_interval, "Check interval (seconds)")
         ->check(CLI::Range(1, 60));
watch_cmd->callback([&watch_port, &watch_interval]() {
    // Implementation
    PortScanner scanner;
    std::cout << "Monitoring port " << watch_port
              << " (Ctrl+C to stop)...\n";

    while (true) {
        auto info = scanner.check_port(static_cast<uint16_t>(watch_port));
        OutputFormatter::print_port_info(info);
        std::cout << "---\n";
        std::this_thread::sleep_for(std::chrono::seconds(watch_interval));
    }
});
```

#### Step 2: Add Tests

```bash
# Add to test.sh
section "Test 11: Watch Command"

info "Testing watch command (5 seconds)"
timeout 5 ./build/zohd watch 3000 --interval 1 > watch-output.txt 2>&1 &
WATCH_PID=$!
sleep 5
kill $WATCH_PID 2>/dev/null || true

if [ -f watch-output.txt ]; then
    if grep -q "Port 3000" watch-output.txt; then
        pass "Watch command produces output"
    else
        fail "Watch command output incorrect"
    fi
    rm watch-output.txt
fi
```

#### Step 3: Update Documentation

```markdown
# README.md

### Monitor Port

```bash
zohd watch 3000
# Continuously monitors port 3000
# Shows status changes in real-time
```

Options:
- `-i, --interval N` - Check every N seconds (default: 1)
```

---

## Platform Implementation

### Adding Windows Support

#### Step 1: Create windows_impl.cpp

```cpp
// src/platform/windows_impl.cpp
#ifdef PLATFORM_WINDOWS

#include "platform_interface.hpp"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

namespace zohd {
namespace platform {

bool is_port_in_use(uint16_t port) {
    PMIB_TCPTABLE_OWNER_PID tcp_table = nullptr;
    DWORD size = 0;

    // Get required size
    GetExtendedTcpTable(nullptr, &size, FALSE, AF_INET,
                        TCP_TABLE_OWNER_PID_ALL, 0);

    tcp_table = (PMIB_TCPTABLE_OWNER_PID)malloc(size);

    if (GetExtendedTcpTable(tcp_table, &size, FALSE, AF_INET,
                            TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR) {
        for (DWORD i = 0; i < tcp_table->dwNumEntries; i++) {
            uint16_t local_port = ntohs((u_short)tcp_table->table[i].dwLocalPort);
            if (local_port == port &&
                tcp_table->table[i].dwState == MIB_TCP_STATE_LISTEN) {
                free(tcp_table);
                return true;
            }
        }
    }

    free(tcp_table);
    return false;
}

// Implement other functions...

} // namespace platform
} // namespace zohd

#endif // PLATFORM_WINDOWS
```

#### Step 2: Update CMakeLists.txt

```cmake
if(WIN32)
    add_definitions(-DPLATFORM_WINDOWS)
    set(PLATFORM_SOURCES src/platform/windows_impl.cpp)
    set(PLATFORM_LIBS ws2_32 iphlpapi psapi)
endif()
```

#### Step 3: Test on Windows

```powershell
# Build
.\build.ps1

# Test
.\build\zohd.exe --help
.\build\zohd.exe check 3000
```

### Adding macOS Support

Similar process, using BSD sockets and `libproc`:

```cpp
// src/platform/macos_impl.cpp
#ifdef PLATFORM_MACOS

#include <sys/sysctl.h>
#include <libproc.h>

// Implementation...

#endif
```

---

## Debugging

### GDB Debugging

```bash
# Build with debug symbols
g++ -g -O0 -DPLATFORM_LINUX [sources] -o zohd-debug

# Run in GDB
gdb ./zohd-debug
(gdb) break main
(gdb) run check 3000
(gdb) step
(gdb) print port_info
(gdb) backtrace
```

### Common Debugging Scenarios

#### Crash on Port Check

```bash
gdb ./zohd-debug
(gdb) run check 3000
# ... crash occurs ...
(gdb) backtrace
(gdb) frame 0
(gdb) print local_variables
```

#### Memory Leaks

```bash
# Run with Valgrind
valgrind --leak-check=full ./build/zohd check 3000

# Expected output (no leaks):
# HEAP SUMMARY:
#     in use at exit: 0 bytes in 0 blocks
#   total heap usage: 100 allocs, 100 frees, 5,000 bytes allocated
#
# All heap blocks were freed -- no leaks are possible
```

#### AddressSanitizer

```bash
# Build with AddressSanitizer
g++ -std=c++17 -O1 -g -fsanitize=address -fno-omit-frame-pointer \
    -DPLATFORM_LINUX [sources] -o zohd-asan

# Run
./zohd-asan check 3000

# Will detect:
# - Use-after-free
# - Heap buffer overflow
# - Stack buffer overflow
# - Memory leaks
```

### Debug Logging

Add to code:

```cpp
#ifdef DEBUG
#define LOG(x) std::cerr << "[DEBUG] " << x << std::endl
#else
#define LOG(x)
#endif

// Usage
LOG("Checking port: " << port);
LOG("Found process: " << process_info.name);
```

Build with:
```bash
g++ -DDEBUG [other flags]
```

---

## Contributing

### Contribution Workflow

1. **Fork** the repository
2. **Clone** your fork:
   ```bash
   git clone https://github.com/YOUR_USERNAME/zohd.git
   ```
3. **Create branch**:
   ```bash
   git checkout -b feature/my-feature
   ```
4. **Make changes** following style guide
5. **Test** thoroughly:
   ```bash
   ./build.sh && ./test.sh
   ```
6. **Commit** with clear messages:
   ```bash
   git commit -m "Add watch command for monitoring ports"
   ```
7. **Push**:
   ```bash
   git push origin feature/my-feature
   ```
8. **Create Pull Request** on GitHub

### Commit Message Format

```
<type>: <short summary>

<detailed description>

<footer>
```

**Types**:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation only
- `style`: Code style changes (formatting)
- `refactor`: Code restructuring
- `test`: Adding tests
- `chore`: Build process, dependencies

**Example**:
```
feat: Add watch command for continuous monitoring

Implements a new `watch` command that continuously monitors
a port and reports status changes.

- Added --interval flag for check frequency
- Updated CLI parser
- Added tests

Closes #42
```

### Pull Request Checklist

- [ ] Code follows style guide
- [ ] All tests pass (`./test.sh`)
- [ ] New tests added for new features
- [ ] Documentation updated (README.md, etc.)
- [ ] No compiler warnings
- [ ] Tested on target platform(s)
- [ ] Commit messages are clear

---

## Release Process

### Version Numbering

Follow Semantic Versioning (semver):
- **MAJOR**: Incompatible API changes
- **MINOR**: New functionality (backward compatible)
- **PATCH**: Bug fixes (backward compatible)

Example: `v1.2.3`

### Release Checklist

1. **Update version** in main.cpp:
   ```cpp
   app{"zohd - Port conflict resolver v1.2.3"};
   ```

2. **Update CHANGELOG.md**:
   ```markdown
   ## [1.2.3] - 2025-01-15

   ### Added
   - Watch command for monitoring

   ### Fixed
   - Permission error handling
   ```

3. **Run all tests**:
   ```bash
   ./build.sh && ./test.sh
   ```

4. **Build release binaries**:
   ```bash
   ./build.sh
   strip build/zohd
   ```

5. **Create git tag**:
   ```bash
   git tag -a v1.2.3 -m "Release v1.2.3"
   git push origin v1.2.3
   ```

6. **Create GitHub release**:
   - Go to GitHub > Releases > New Release
   - Choose tag: v1.2.3
   - Title: zohd v1.2.3
   - Description: Copy from CHANGELOG.md
   - Attach binaries (zohd-linux, zohd-macos, zohd-windows.exe)

7. **Publish**:
   - Update package managers (brew, apt, etc.)
   - Announce on social media/forums

---

## Appendix: Useful Commands

### Code Quality

```bash
# Format all source files
find src/ -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

# Static analysis
cppcheck --enable=all --inconclusive src/

# Count lines of code
cloc src/ include/
```

### Performance Profiling

```bash
# Build with profiling
g++ -pg [flags] -o zohd-prof

# Run
./zohd-prof scan

# Generate report
gprof zohd-prof gmon.out > analysis.txt
```

### Documentation Generation

```bash
# Generate Doxygen docs
doxygen Doxyfile

# View
firefox docs/html/index.html
```

---

## Getting Help

- **Issues**: https://github.com/yourusername/zohd/issues
- **Discussions**: https://github.com/yourusername/zohd/discussions
- **Email**: dev@zohd.dev

---

**Happy Coding!** 🚀
