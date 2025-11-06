#include <iostream>
#include <string>
#include "CLI11.hpp"
#include "core/port_scanner.hpp"
#include "core/process_manager.hpp"
#include "cli/output_formatter.hpp"

using namespace zohd;

int main(int argc, char** argv) {
    CLI::App app{"zohd - Port conflict resolver v1.0.0"};
    app.require_subcommand(1);

    // SCAN command
    auto* scan_cmd = app.add_subcommand("scan", "Scan common development ports");
    scan_cmd->callback([]() {
        PortScanner scanner;
        auto results = scanner.scan_dev_ports();
        OutputFormatter::print_scan_results(results);
    });

    // CHECK command
    auto* check_cmd = app.add_subcommand("check", "Check if specific port is in use");
    int check_port = 0;
    check_cmd->add_option("port", check_port, "Port number to check")
             ->required()
             ->check(CLI::Range(1, 65535));
    check_cmd->callback([&check_port]() {
        PortScanner scanner;
        auto info = scanner.check_port(static_cast<uint16_t>(check_port));
        OutputFormatter::print_port_info(info);
    });

    // KILL command
    auto* kill_cmd = app.add_subcommand("kill", "Kill process using port");
    int kill_port = 0;
    bool force = false;
    kill_cmd->add_option("port", kill_port, "Port number")
            ->required()
            ->check(CLI::Range(1, 65535));
    kill_cmd->add_flag("-f,--force", force, "Force kill without confirmation");
    kill_cmd->callback([&kill_port, &force]() {
        PortScanner scanner;
        auto info = scanner.check_port(static_cast<uint16_t>(kill_port));

        if (!info.is_in_use()) {
            std::cout << "Port " << kill_port << " is not in use\n";
            return;
        }

        if (!info.process) {
            std::cerr << "Could not find process information for port " << kill_port << "\n";
            return;
        }

        if (!force) {
            OutputFormatter::print_port_info(info);
            std::cout << "\nKill this process? (y/N): ";
            std::string response;
            std::getline(std::cin, response);
            if (response != "y" && response != "Y") {
                std::cout << "Cancelled\n";
                return;
            }
        }

        ProcessManager pm;
        if (pm.terminate_process(info.process->pid)) {
            std::cout << "Process killed successfully\n";
        } else {
            std::cerr << "Failed to kill process (permission denied?)\n";
        }
    });

    // SUGGEST command
    auto* suggest_cmd = app.add_subcommand("suggest", "Suggest free ports");
    int count = 5;
    suggest_cmd->add_option("-n,--count", count, "Number of ports to suggest")
               ->check(CLI::Range(1, 20));
    suggest_cmd->callback([&count]() {
        PortScanner scanner;
        auto ports = scanner.suggest_free_ports(static_cast<size_t>(count));
        OutputFormatter::print_suggested_ports(ports);
    });

    // LIST command
    auto* list_cmd = app.add_subcommand("list", "List all active ports");
    list_cmd->callback([]() {
        PortScanner scanner;
        auto ports = scanner.get_all_active_ports();
        OutputFormatter::print_active_ports(ports);
    });

    // INFO command
    auto* info_cmd = app.add_subcommand("info", "Detailed port information");
    int info_port = 0;
    info_cmd->add_option("port", info_port, "Port number")
            ->required()
            ->check(CLI::Range(1, 65535));
    info_cmd->callback([&info_port]() {
        PortScanner scanner;
        auto info = scanner.check_port(static_cast<uint16_t>(info_port));
        OutputFormatter::print_detailed_info(info);
    });

    // FIX command (interactive)
    auto* fix_cmd = app.add_subcommand("fix", "Interactive port conflict resolution");
    int fix_port = 0;
    fix_cmd->add_option("port", fix_port, "Port number")
           ->required()
           ->check(CLI::Range(1, 65535));
    fix_cmd->callback([&fix_port]() {
        PortScanner scanner;
        auto info = scanner.check_port(static_cast<uint16_t>(fix_port));

        if (!info.is_in_use()) {
            std::cout << "Port " << fix_port << " is FREE\n";
            return;
        }

        OutputFormatter::print_port_info(info);
        std::cout << "\nChoose action:\n";
        std::cout << "1. Kill the process\n";
        std::cout << "2. Use alternative port (suggest free port)\n";
        std::cout << "3. Show detailed process info\n";
        std::cout << "4. Cancel\n\n";
        std::cout << "Enter choice (1-4): ";

        int choice;
        if (!(std::cin >> choice)) {
            std::cout << "Invalid input\n";
            return;
        }

        switch(choice) {
            case 1: {
                if (!info.process) {
                    std::cerr << "Could not find process information\n";
                    break;
                }
                ProcessManager pm;
                if (pm.terminate_process(info.process->pid)) {
                    std::cout << "Process killed successfully\n";
                } else {
                    std::cerr << "Failed to kill process (permission denied?)\n";
                }
                break;
            }
            case 2: {
                auto suggestions = scanner.suggest_free_ports(3);
                std::cout << "\nAvailable alternative ports:\n";
                for (auto port : suggestions) {
                    std::cout << "  " << port << "\n";
                }
                break;
            }
            case 3:
                OutputFormatter::print_detailed_info(info);
                break;
            default:
                std::cout << "Cancelled\n";
        }
    });

    try {
        CLI11_PARSE(app, argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    return 0;
}
