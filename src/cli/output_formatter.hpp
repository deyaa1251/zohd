#pragma once

#include "../core/port_info.hpp"
#include <vector>
#include <iostream>

namespace zohd {

class OutputFormatter {
public:
    static void print_scan_results(const std::vector<PortInfo>& results);
    static void print_port_info(const PortInfo& info);
    static void print_detailed_info(const PortInfo& info);
    static void print_suggested_ports(const std::vector<uint16_t>& ports);
    static void print_active_ports(const std::vector<PortInfo>& ports);

private:
    static std::string status_symbol(PortStatus status);
    static std::string format_uptime(uint64_t start_time);
};

} // namespace zohd
