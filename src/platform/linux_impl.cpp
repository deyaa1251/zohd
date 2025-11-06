#ifdef PLATFORM_LINUX

#include "platform_interface.hpp"
#include <fstream>
#include <sstream>
#include <ctime>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include <pwd.h>
#include <algorithm>
#include <cctype>

namespace zohd {
namespace platform {

// Helper function to find PID by socket inode
static uint32_t find_pid_by_inode(unsigned long inode);

// Helper function to trim whitespace
static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

bool is_port_in_use(uint16_t port) {
    // Check both IPv4 and IPv6
    std::vector<std::string> tcp_files = {"/proc/net/tcp", "/proc/net/tcp6"};

    for (const auto& tcp_file : tcp_files) {
        std::ifstream file(tcp_file);
        if (!file.is_open()) continue;

        std::string line;
        std::getline(file, line);  // Skip header

        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string sl;  // Read as string since it has a colon (e.g., "0:")
            std::string local_address, rem_address, st;

            iss >> sl >> local_address >> rem_address >> st;

            // Parse local address (format: "0100007F:0BB8" for 127.0.0.1:3000)
            size_t colon_pos = local_address.find(':');
            if (colon_pos != std::string::npos && colon_pos + 1 < local_address.size()) {
                std::string port_hex = local_address.substr(colon_pos + 1);
                if (!port_hex.empty()) {
                    try {
                        uint16_t local_port = std::stoul(port_hex, nullptr, 16);

                        // Check if LISTENING (state 0A)
                        if (local_port == port && st == "0A") {
                            return true;
                        }
                    } catch (const std::exception&) {
                        // Invalid port format, skip this entry
                        continue;
                    }
                }
            }
        }
    }

    return false;
}

std::vector<PortInfo> get_tcp_connections() {
    std::vector<PortInfo> result;
    std::vector<std::string> tcp_files = {"/proc/net/tcp", "/proc/net/tcp6"};

    for (const auto& tcp_file : tcp_files) {
        std::ifstream file(tcp_file);
        if (!file.is_open()) continue;

        std::string line;
        std::getline(file, line);  // Skip header

        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string sl;  // Read as string since it has a colon (e.g., "0:")
            std::string local_address, rem_address, st;
            unsigned long inode = 0;
            int uid;

            // Parse line: sl local_address rem_address st tx_queue rx_queue tr tm->when retrnsmt uid timeout inode
            iss >> sl >> local_address >> rem_address >> st;

            // Skip tx_queue:rx_queue, tr, tm->when, retrnsmt
            std::string skip;
            for (int i = 0; i < 4; i++) iss >> skip;

            iss >> uid >> skip >> inode;

            // Only LISTENING state (0A)
            if (st == "0A") {
                size_t colon_pos = local_address.find(':');
                if (colon_pos != std::string::npos && colon_pos + 1 < local_address.size()) {
                    std::string port_hex = local_address.substr(colon_pos + 1);
                    if (!port_hex.empty()) {
                        try {
                            uint16_t port_num = std::stoul(port_hex, nullptr, 16);

                            PortInfo info;
                            info.port = port_num;
                            info.status = PortStatus::IN_USE;

                            // Find PID by inode
                            uint32_t pid = find_pid_by_inode(inode);
                            if (pid > 0) {
                                info.process = get_process_info(pid);
                            }

                            result.push_back(info);
                        } catch (const std::exception&) {
                            // Invalid port format, skip this entry
                            continue;
                        }
                    }
                }
            }
        }
    }

    return result;
}

static uint32_t find_pid_by_inode(unsigned long inode) {
    if (inode == 0) return 0;

    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) return 0;

    struct dirent* entry;
    while ((entry = readdir(proc_dir))) {
        if (entry->d_type == DT_DIR && std::isdigit(entry->d_name[0])) {
            uint32_t pid = 0;
            try {
                pid = std::stoul(entry->d_name);
            } catch (const std::exception&) {
                continue;
            }

            std::string fd_path = std::string("/proc/") + entry->d_name + "/fd";
            DIR* fd_dir = opendir(fd_path.c_str());
            if (fd_dir) {
                struct dirent* fd_entry;
                while ((fd_entry = readdir(fd_dir))) {
                    std::string fd_name = fd_entry->d_name;
                    if (fd_name == "." || fd_name == "..") continue;

                    char link_target[256];
                    std::string link_path = fd_path + "/" + fd_name;
                    ssize_t len = readlink(link_path.c_str(), link_target, sizeof(link_target) - 1);
                    if (len > 0) {
                        link_target[len] = '\0';
                        std::string target(link_target);

                        // Check if matches "socket:[inode]"
                        std::string search = "socket:[" + std::to_string(inode) + "]";
                        if (target == search) {
                            closedir(fd_dir);
                            closedir(proc_dir);
                            return pid;
                        }
                    }
                }
                closedir(fd_dir);
            }
        }
    }

    closedir(proc_dir);
    return 0;
}

ProcessInfo get_process_info(uint32_t pid) {
    ProcessInfo info;
    info.pid = pid;
    info.start_time = 0;
    info.name = "unknown";
    info.command_line = "";
    info.user = "";

    // Read /proc/<pid>/cmdline
    std::string cmdline_path = "/proc/" + std::to_string(pid) + "/cmdline";
    std::ifstream cmdline_file(cmdline_path);
    if (cmdline_file) {
        std::string cmdline;
        std::getline(cmdline_file, cmdline, '\0');
        if (!cmdline.empty()) {
            info.command_line = cmdline;

            // Extract process name from command
            size_t last_slash = cmdline.find_last_of('/');
            if (last_slash != std::string::npos) {
                info.name = cmdline.substr(last_slash + 1);
                // Remove any null terminators or spaces
                size_t space_pos = info.name.find_first_of(" \0");
                if (space_pos != std::string::npos) {
                    info.name = info.name.substr(0, space_pos);
                }
            } else {
                info.name = cmdline;
                size_t space_pos = info.name.find_first_of(" \0");
                if (space_pos != std::string::npos) {
                    info.name = info.name.substr(0, space_pos);
                }
            }
        }
    }

    // Read /proc/<pid>/stat for start time
    std::string stat_path = "/proc/" + std::to_string(pid) + "/stat";
    std::ifstream stat_file(stat_path);
    if (stat_file) {
        std::string line;
        std::getline(stat_file, line);

        // Parse stat file (22nd field is starttime in jiffies since boot)
        // Format: pid (comm) state ppid pgrp session tty_nr tpgid flags ...
        size_t last_paren = line.rfind(')');
        if (last_paren != std::string::npos) {
            std::istringstream iss(line.substr(last_paren + 1));
            std::string field;
            int field_num = 3;  // We've already parsed pid and (comm)

            // Field 22 is starttime (we need to skip to field 22)
            while (field_num < 22 && iss >> field) {
                field_num++;
            }

            if (field_num == 22 && !field.empty()) {
                try {
                    unsigned long long starttime_jiffies = std::stoull(field);

                    // Get system boot time
                    std::ifstream uptime_file("/proc/uptime");
                    if (uptime_file) {
                        double uptime_seconds;
                        uptime_file >> uptime_seconds;

                        // Convert jiffies to seconds (assuming 100 jiffies per second)
                        long clock_ticks = sysconf(_SC_CLK_TCK);
                        double process_start_seconds = static_cast<double>(starttime_jiffies) / clock_ticks;

                        // Calculate absolute start time
                        auto now = std::time(nullptr);
                        info.start_time = now - static_cast<uint64_t>(uptime_seconds - process_start_seconds);
                    }
                } catch (const std::exception&) {
                    // Failed to parse start time, leave it as 0
                }
            }
        }
    }

    // Read /proc/<pid>/status for UID
    std::string status_path = "/proc/" + std::to_string(pid) + "/status";
    std::ifstream status_file(status_path);
    if (status_file) {
        std::string line;
        while (std::getline(status_file, line)) {
            if (line.substr(0, 4) == "Uid:") {
                std::istringstream iss(line);
                std::string label;
                uid_t uid;
                iss >> label >> uid;

                struct passwd* pw = getpwuid(uid);
                if (pw) {
                    info.user = pw->pw_name;
                } else {
                    info.user = std::to_string(uid);
                }
                break;
            }
        }
    }

    return info;
}

bool terminate_process(uint32_t pid) {
    return kill(static_cast<pid_t>(pid), SIGTERM) == 0;
}

bool force_kill_process(uint32_t pid) {
    return kill(static_cast<pid_t>(pid), SIGKILL) == 0;
}

std::string get_current_user() {
    uid_t uid = getuid();
    struct passwd* pw = getpwuid(uid);
    if (pw) {
        return pw->pw_name;
    }
    return "unknown";
}

bool is_process_alive(uint32_t pid) {
    // Send signal 0 to check if process exists
    return kill(static_cast<pid_t>(pid), 0) == 0;
}

} // namespace platform
} // namespace zohd

#endif // PLATFORM_LINUX
