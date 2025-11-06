#include "output_formatter.hpp"
#include <iomanip>
#include <ctime>
#include <sstream>

namespace zohd {

void OutputFormatter::print_scan_results(const std::vector<PortInfo>& results) {
    std::cout << "Scanning common development ports...\n\n";

    int free_count = 0;
    int busy_count = 0;

    for (const auto& info : results) {
        std::cout << status_symbol(info.status) << " "
                  << std::setw(5) << info.port << " - ";

        if (info.is_free()) {
            std::cout << "FREE\n";
            free_count++;
        } else {
            std::cout << "USED by " << (info.process ? info.process->name : "unknown")
                      << " (PID " << (info.process ? std::to_string(info.process->pid) : "?") << ")";
            if (info.process && info.process->start_time > 0) {
                std::cout << " [" << format_uptime(info.process->start_time) << "]";
            }
            std::cout << "\n";
            busy_count++;
        }
    }

    std::cout << "\nSummary: " << busy_count << " ports busy, "
              << free_count << " ports free\n";
}

void OutputFormatter::print_port_info(const PortInfo& info) {
    if (info.is_free()) {
        std::cout << "Port " << info.port << " is FREE\n";
    } else {
        std::cout << "Port " << info.port << " is IN USE\n";
        if (info.process) {
            std::cout << "  Process: " << info.process->name << "\n";
            std::cout << "  PID: " << info.process->pid << "\n";
            if (!info.process->command_line.empty()) {
                std::cout << "  Command: " << info.process->command_line << "\n";
            }
            if (!info.process->user.empty()) {
                std::cout << "  User: " << info.process->user << "\n";
            }
            if (info.process->start_time > 0) {
                std::cout << "  Started: " << format_uptime(info.process->start_time) << "\n";
            }
        }
    }
}

void OutputFormatter::print_detailed_info(const PortInfo& info) {
    std::cout << "Port " << info.port << " Information:\n";
    std::cout << "  Status: " << (info.is_free() ? "FREE" : "IN USE") << "\n";

    if (info.is_in_use() && info.process) {
        std::cout << "  Process: " << info.process->name << "\n";
        std::cout << "  PID: " << info.process->pid << "\n";
        if (!info.process->user.empty()) {
            std::cout << "  User: " << info.process->user << "\n";
        }
        if (!info.process->command_line.empty()) {
            std::cout << "  Command: " << info.process->command_line << "\n";
        }
        if (info.process->start_time > 0) {
            std::cout << "  Started: " << format_uptime(info.process->start_time) << "\n";
        }
    }
}

void OutputFormatter::print_suggested_ports(const std::vector<uint16_t>& ports) {
    if (ports.empty()) {
        std::cout << "No free ports found in development ranges.\n";
        return;
    }

    std::cout << "Available ports in development ranges:\n";
    for (auto port : ports) {
        std::cout << "  " << port;

        // Add common use hints
        if (port == 3000) {
            std::cout << " - Commonly used for React/Node";
        } else if (port == 5000) {
            std::cout << " - Common for Flask/Go";
        } else if (port == 8080) {
            std::cout << " - Alternative HTTP port";
        }

        std::cout << "\n";
    }
}

void OutputFormatter::print_active_ports(const std::vector<PortInfo>& ports) {
    if (ports.empty()) {
        std::cout << "No active ports found.\n";
        return;
    }

    std::cout << "Active ports:\n";
    std::cout << std::left
              << std::setw(8) << "PORT"
              << std::setw(8) << "PID"
              << std::setw(20) << "PROCESS"
              << std::setw(30) << "COMMAND"
              << "USER\n";
    std::cout << std::string(80, '-') << "\n";

    for (const auto& info : ports) {
        if (!info.is_in_use() || !info.process) continue;

        std::string proc_name = info.process->name.substr(0, 19);
        std::string cmd_line = info.process->command_line.substr(0, 29);

        std::cout << std::left
                  << std::setw(8) << info.port
                  << std::setw(8) << info.process->pid
                  << std::setw(20) << proc_name
                  << std::setw(30) << cmd_line
                  << info.process->user << "\n";
    }

    std::cout << "\nTotal: " << ports.size() << " active ports\n";
}

std::string OutputFormatter::status_symbol(PortStatus status) {
    switch(status) {
        case PortStatus::FREE: return "✓";
        case PortStatus::IN_USE: return "✗";
        default: return "?";
    }
}

std::string OutputFormatter::format_uptime(uint64_t start_time) {
    if (start_time == 0) {
        return "unknown";
    }

    auto now = std::time(nullptr);
    auto elapsed = now - static_cast<std::time_t>(start_time);

    if (elapsed < 0) {
        return "unknown";
    }

    if (elapsed < 60) {
        return std::to_string(elapsed) + "s ago";
    } else if (elapsed < 3600) {
        return std::to_string(elapsed / 60) + "m ago";
    } else if (elapsed < 86400) {
        return std::to_string(elapsed / 3600) + "h ago";
    } else {
        return std::to_string(elapsed / 86400) + "d ago";
    }
}

} // namespace zohd
