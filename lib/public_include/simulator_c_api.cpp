#include "simulator_c_api.h"

#include "simulator.hpp"

#include <exception>
#include <memory>
#include <new>
#include <string>

struct ssp4sim_simulator_handle
{
    std::unique_ptr<ssp4sim::Simulator> simulator;
};

namespace
{
    thread_local std::string g_last_error;

    int set_error(const int status, const std::string &message)
    {
        g_last_error = message;
        return status;
    }
}

int ssp4sim_simulator_create(
    const char *config_path,
    ssp4sim_simulator_handle **out_handle
)
{
    if (out_handle == nullptr)
    {
        return set_error(
            SSP4SIM_STATUS_INVALID_ARGUMENT,
            "out_handle must not be null"
        );
    }
    *out_handle = nullptr;

    if (config_path == nullptr)
    {
        return set_error(
            SSP4SIM_STATUS_INVALID_ARGUMENT,
            "config_path must not be null"
        );
    }

    try
    {
        auto *handle = new ssp4sim_simulator_handle();
        handle->simulator = std::make_unique<ssp4sim::Simulator>(std::string(config_path));
        *out_handle = handle;
        g_last_error.clear();
        return SSP4SIM_STATUS_OK;
    }
    catch (const std::exception &ex)
    {
        return set_error(SSP4SIM_STATUS_RUNTIME_ERROR, ex.what());
    }
    catch (...)
    {
        return set_error(
            SSP4SIM_STATUS_RUNTIME_ERROR,
            "Unknown error while creating simulator"
        );
    }
}

int ssp4sim_simulator_init(ssp4sim_simulator_handle *handle)
{
    if (handle == nullptr || !handle->simulator)
    {
        return set_error(
            SSP4SIM_STATUS_INVALID_ARGUMENT,
            "handle must not be null"
        );
    }

    try
    {
        handle->simulator->init();
        g_last_error.clear();
        return SSP4SIM_STATUS_OK;
    }
    catch (const std::exception &ex)
    {
        return set_error(SSP4SIM_STATUS_RUNTIME_ERROR, ex.what());
    }
    catch (...)
    {
        return set_error(
            SSP4SIM_STATUS_RUNTIME_ERROR,
            "Unknown error while initializing simulator"
        );
    }
}

int ssp4sim_simulator_simulate(ssp4sim_simulator_handle *handle)
{
    if (handle == nullptr || !handle->simulator)
    {
        return set_error(
            SSP4SIM_STATUS_INVALID_ARGUMENT,
            "handle must not be null"
        );
    }

    try
    {
        handle->simulator->simulate();
        g_last_error.clear();
        return SSP4SIM_STATUS_OK;
    }
    catch (const std::exception &ex)
    {
        return set_error(SSP4SIM_STATUS_RUNTIME_ERROR, ex.what());
    }
    catch (...)
    {
        return set_error(
            SSP4SIM_STATUS_RUNTIME_ERROR,
            "Unknown error while running simulation"
        );
    }
}

void ssp4sim_simulator_destroy(ssp4sim_simulator_handle *handle)
{
    delete handle;
}

const char *ssp4sim_last_error(void)
{
    return g_last_error.c_str();
}
