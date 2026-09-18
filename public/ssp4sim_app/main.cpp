#include "ssp4cpp/utils/log.hpp"

#include "simulator.hpp"

#include <iostream>
#include <string>

#ifndef SSP4SIM_APP_VERSION
#define SSP4SIM_APP_VERSION "unknown"
#endif

static const char *SSP4SIM_DOCS_USAGE =
    "https://github.com/jkCXf9X4/ssp4sim/blob/main/docs/usage.md";
static const char *SSP4SIM_DOCS_CONFIGURATION =
    "https://github.com/jkCXf9X4/ssp4sim/blob/main/docs/configuration.md";

namespace
{
    void print_help(const char *program_name)
    {
        std::cout << "Usage: " << program_name << " <config.json>\n"
                  << "       " << program_name << " -h | --help\n"
                  << "       " << program_name << " -v | --version\n"
                  << "\n"
                  << "Run an SSP4SIM simulation from a JSON configuration file.\n"
                  << "\n"
                  << "Arguments:\n"
                  << "  config.json          Path to the simulation JSON configuration file.\n"
                  << "\n"
                  << "Options:\n"
                  << "  -h, --help           Show this help message and exit.\n"
                  << "  -v, --version        Show the embedded CLI version and exit.\n"
                  << "\n"
                  << "Documentation:\n"
                  << "  " << SSP4SIM_DOCS_USAGE << "\n"
                  << "                        Usage, examples, and result artifacts.\n"
                  << "  " << SSP4SIM_DOCS_CONFIGURATION << "\n"
                  << "                        Full JSON configuration key reference.\n";
    }
}

int main(int argc, char *argv[])
{
    if (argc == 2)
    {
        const std::string arg = argv[1];

        if (arg == "-h" || arg == "--help")
        {
            print_help(argv[0]);
            return 0;
        }

        if (arg == "-v" || arg == "--version")
        {
            std::cout << SSP4SIM_APP_VERSION << '\n';
            return 0;
        }
    }

    ssp4cpp::utils::log::Logger* log = ssp4cpp::utils::log::simple_logger();
    LOG_INFO(log, "---SSP4SIM---");

    std::string config_path;

    if (argc == 2)
    {
        config_path = std::string(argv[1]);
        LOG_INFO(log, "[{}] Config: {}", __func__, config_path);
    }
    else
    {
        print_help(argv[0]);
        LOG_ERROR(log, "Need a config path, --help, or --version");
        return 1;
    }

    auto sim = ssp4sim::Simulator(config_path);

    sim.init();
    sim.simulate();

    LOG_TRACE_L1(log, "[{}] exit", __func__);
    return 0;
}
