#include "ssp4cpp/utils/log.hpp"

#include "simulator.hpp"

#include <iostream>
#include <string>

#ifndef SSP4SIM_APP_VERSION
#define SSP4SIM_APP_VERSION "unknown"
#endif

namespace
{
    void print_usage(const char *program_name)
    {
        std::cerr << "Usage: " << program_name << " <config.json>\n"
                  << "       " << program_name << " --version\n";
    }
}

int main(int argc, char *argv[])
{
    if (argc == 2 && std::string(argv[1]) == "--version")
    {
        std::cout << SSP4SIM_APP_VERSION << '\n';
        return 0;
    }

    quill::Logger* log = ssp4cpp::utils::log::simple_logger();
    LOG_INFO(log, "---SSP4SIM---");

    std::string config_path;

    if (argc == 2)
    {
        config_path = std::string(argv[1]);
        LOG_INFO(log, "[{}] Config: {}", __func__, config_path);
    }
    else
    {
        print_usage(argv[0]);
        LOG_ERROR(log, "Need a config path or --version");
        return 1;
    }

    auto sim = ssp4sim::Simulator(config_path);

    sim.init();
    sim.simulate();

    LOG_TRACE_L1(log, "[{}] exit", __func__);
    return 0;
}
