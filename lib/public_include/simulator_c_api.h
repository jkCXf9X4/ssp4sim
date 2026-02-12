#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ssp4sim_simulator_handle ssp4sim_simulator_handle;

enum ssp4sim_status
{
    SSP4SIM_STATUS_OK = 0,
    SSP4SIM_STATUS_INVALID_ARGUMENT = 1,
    SSP4SIM_STATUS_RUNTIME_ERROR = 2,
    SSP4SIM_STATUS_OUT_OF_MEMORY = 3
};

int ssp4sim_simulator_create(
    const char *config_path,
    ssp4sim_simulator_handle **out_handle
);

int ssp4sim_simulator_init(ssp4sim_simulator_handle *handle);

int ssp4sim_simulator_simulate(ssp4sim_simulator_handle *handle);

void ssp4sim_simulator_destroy(ssp4sim_simulator_handle *handle);

const char *ssp4sim_last_error(void);

#ifdef __cplusplus
}
#endif
