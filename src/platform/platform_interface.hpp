#pragma once

#include "../core/port_info.hpp"
#include <vector>
#include <cstdint>

namespace zohd {
namespace platform {

// Platform-specific port checking
bool is_port_in_use(uint16_t port);

// Get all TCP connections with process info
std::vector<PortInfo> get_tcp_connections();

// Get process info by PID
ProcessInfo get_process_info(uint32_t pid);

// Kill process (graceful)
bool terminate_process(uint32_t pid);

// Kill process (force)
bool force_kill_process(uint32_t pid);

// Get current username
std::string get_current_user();

// Check if process is alive
bool is_process_alive(uint32_t pid);

} // namespace platform
} // namespace zohd
