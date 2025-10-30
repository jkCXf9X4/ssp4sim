#include "cutecpp/log.hpp"

#include "simulator.hpp"

int main(int argc, char *argv[])
{
    auto log = Logger("main", LogLevel::debug);
    log(ext_trace)("[{}] SSP4CPP", __func__);

    std::string config_path;

    if (argc == 2)
    {
        config_path = std::string(argv[1]);
        log(info)("[{}] Config: {}", __func__, config_path);
    }
    else
    {
        log(error)("Need an argument for the config to execute");
        return 1;
    }

    auto sim = ssp4sim::Simulator(config_path);

    sim.init();
    sim.simulate();

    log(ext_trace)("[{}] exit", __func__);
    return 0;
}
