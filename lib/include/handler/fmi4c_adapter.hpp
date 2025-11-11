#pragma once

#include "cutecpp/log.hpp"

#include <fmi4c.h>

#include <string>
#include <filesystem>

namespace ssp4sim::handler
{
    class FmuInstance
    {
    public:
        Logger log = Logger("ssp4sim.handler.FmuInstance", LogLevel::info);

        FmuInstance(const std::filesystem::path &path, std::string instance_name);

        ~FmuInstance();

        FmuInstance(const FmuInstance &) = delete;
        FmuInstance &operator=(const FmuInstance &) = delete;

        bool supports_co_simulation() const;

        fmuHandle *raw();

        [[nodiscard]] fmiVersion_t version() const;

        [[nodiscard]] const std::string &path() const;

        [[nodiscard]] const std::string &instance_name() const;

    private:
        std::string fmu_path_;
        std::string instance_name_;
        fmuHandle *handle_ = nullptr;
        fmiVersion_t version_ = fmiVersionUnknown;
    };

    class CoSimulationModel
    {
        FmuInstance &instance_;
        bool instantiated_ = false;
        uint64_t current_time_ = 0;
        fmi2Status last_status_ = fmi2OK;

    public:
        Logger log = Logger("ssp4sim.handler.CoSimulationModel", LogLevel::info);

        CoSimulationModel(FmuInstance &instance);

        ~CoSimulationModel();

        CoSimulationModel(const CoSimulationModel &) = delete;
        CoSimulationModel &operator=(const CoSimulationModel &) = delete;

        fmi2InstanceHandle *handle;

        bool instantiate(bool visible, bool logging_on);

        bool setup_experiment(uint64_t start_time, uint64_t stop_time, double tolerance);

        bool enter_initialization_mode();

        bool exit_initialization_mode();

        uint64_t step_until(uint64_t stop_time);

        bool step(uint64_t step_size);

        bool terminate();

        [[nodiscard]] uint64_t get_simulation_time() const;

        [[nodiscard]] fmi2Status last_status() const;

        bool set_real_input_derivative(uint64_t value_reference, int derivative_order, double value);

        bool get_real_output_derivative(uint64_t value_reference, int derivative_order, double &out);

        bool read_real(uint64_t value_reference, double &out);

        bool read_integer(uint64_t value_reference, int &out);

        bool read_boolean(uint64_t value_reference, int &out);

        bool read_string(uint64_t value_reference, std::string &out);

        bool write_real(uint64_t value_reference, double value);

        bool write_integer(uint64_t value_reference, int value);

        bool write_boolean(uint64_t value_reference, int value);

        bool write_string(uint64_t value_reference, const std::string &value);
    };
}
