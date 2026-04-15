#include "cutecpp/log.hpp"

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

    auto log = Logger("main", LogLevel::debug);
    log(info)("---SSP4SIM---", __func__);

    std::string config_path;

    if (argc == 2)
    {
        config_path = std::string(argv[1]);
        log(info)("[{}] Config: {}", __func__, config_path);
    }
    else
    {
        print_usage(argv[0]);
        log(error)("Need a config path or --version");
        return 1;
    }

    auto sim = ssp4sim::Simulator(config_path);

    sim.init();
    sim.simulate();

    log(ext_trace)("[{}] exit", __func__);
    return 0;
}
