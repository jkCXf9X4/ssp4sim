#include "handler/fmi4c_adapter.hpp"

#include "utils/time.hpp"

#include <cstdlib>
#include <stdexcept>
#include <string>
#include <utility>


namespace ssp4sim::handler
{
    namespace detail
    {
        std::once_flag fmi4c_message_flag;
        thread_local std::string last_fmi4c_message;

        void fmi4c_message_callback(const char *message)
        {
            if (message != nullptr)
            {
                last_fmi4c_message = message;
            }
        }

        void ensure_message_callback_registered()
        {
            std::call_once(fmi4c_message_flag, []()
                           { fmi4c_setMessageFunction(&fmi4c_message_callback); });
        }

        void clear_last_message()
        {
            last_fmi4c_message.clear();
        }

        std::string consume_last_message()
        {
            auto message = last_fmi4c_message;
            last_fmi4c_message.clear();
            return message;
        }
    }

    bool status_ok(fmi2Status status)
    {
        if (status == fmi2OK)
        {
            return true;
        }

        throw std::runtime_error("Failed status check");
    }

    FmuInstance::FmuInstance(const std::filesystem::path &path, std::string instance_name)
    {
        fmu_path_ = path.string();
        instance_name_ = std::move(instance_name);

        log(debug)("[{}] Loading FMU {}", __func__, fmu_path_);
        detail::ensure_message_callback_registered();
        detail::clear_last_message();
        handle_ = fmi4c_loadFmu(fmu_path_.c_str(), instance_name_.c_str());
        if (handle_ == nullptr)
        {
            auto message = detail::consume_last_message();
            throw std::runtime_error(Logger::format("Failed to load FMU '{}': {}", fmu_path_, message.empty() ? "unknown error" : message));
        }

        version_ = fmi4c_getFmiVersion(handle_);
        if (version_ != fmiVersion2)
        {
            throw std::runtime_error(Logger::format("Unsupported FMI version {} for FMU '{}'", static_cast<int>(version_), fmu_path_));
        }
    }

    FmuInstance::~FmuInstance()
    {
        if (handle_ != nullptr)
        {
            log(debug)("[{}] Freeing FMU {}", __func__, fmu_path_);
            fmi4c_freeFmu(handle_);
            handle_ = nullptr;
        }
    }

    bool FmuInstance::supports_co_simulation() const
    {
        return fmi2_getSupportsCoSimulation(handle_) == true;
    }

    fmuHandle *FmuInstance::raw()
    {
        return handle_;
    }

    fmiVersion_t FmuInstance::version() const
    {
        return version_;
    }

    const std::string &FmuInstance::path() const
    {
        return fmu_path_;
    }

    const std::string &FmuInstance::instance_name() const
    {
        return instance_name_;
    }

    // CoSimulationModel ----------------------------

    CoSimulationModel::CoSimulationModel(FmuInstance &instance) : instance_(instance)
    {
    }

    CoSimulationModel::~CoSimulationModel()
    {
        terminate();
    }

    bool CoSimulationModel::instantiate(bool visible, bool logging_on)
    {
        if (instantiated_)
        {
            return true;
        }

        if (instance_.version() != fmiVersion2)
        {
            throw std::runtime_error("Only FMI 2.0 FMUs are supported");
        }

        if (!instance_.supports_co_simulation())
        {
            throw std::runtime_error(Logger::format("FMU '{}' does not support co-simulation", instance_.path()));
        }

        log(debug)("[{}] Instantiating FMU {}", __func__, instance_.path());
        detail::ensure_message_callback_registered();
        detail::clear_last_message();
        handle = fmi2_instantiate(instance_.raw(),
                                  fmi2CoSimulation,
                                  nullptr,
                                  ::calloc,
                                  ::free,
                                  nullptr,
                                  nullptr,
                                  visible ? fmi2True : fmi2False,
                                  logging_on ? fmi2True : fmi2False);

        bool success = handle != nullptr;
        instantiated_ = success;
        last_status_ = success ? fmi2OK : fmi2Error;
        current_time_ = 0;

        if (!success)
        {
            auto message = detail::consume_last_message();
            throw std::runtime_error(Logger::format("Failed to instantiate FMU '{}': {}", instance_.path(), message.empty() ? "unknown error" : message));
        }

        return success;
    }

    bool CoSimulationModel::setup_experiment(uint64_t start_time, uint64_t stop_time, double tolerance)
    {
        if (!instantiated_)
        {
            throw std::logic_error("setup_experiment called before instantiate");
        }

        if (start_time < 1000)
        {
            start_time = 0;
        }

        auto stop_defined = stop_time > start_time ? fmi2True : fmi2False;
        auto tolerance_defined = tolerance > 0.0 ? fmi2True : fmi2False;

        double start = utils::time::ns_to_s(start_time);
        double stop = utils::time::ns_to_s(stop_time);

        log(debug)("[{}] setup_experiment start:{} stop:{} tolerance:{}", __func__, start, stop, tolerance);

        last_status_ = fmi2_setupExperiment(handle, tolerance_defined, tolerance, start, stop_defined, stop);
        if (status_ok(last_status_))
        {
            current_time_ = start_time;
            log(debug)("[{}] start:{}", __func__, current_time_);
        }
        return status_ok(last_status_);
    }

    bool CoSimulationModel::enter_initialization_mode()
    {
        if (!instantiated_)
        {
            throw std::logic_error("enter_initialization_mode called before instantiate");
        }

        last_status_ = fmi2_enterInitializationMode(handle);
        return status_ok(last_status_);
    }

    bool CoSimulationModel::exit_initialization_mode()
    {
        if (!instantiated_)
        {
            throw std::logic_error("exit_initialization_mode called before instantiate");
        }

        last_status_ = fmi2_exitInitializationMode(handle);
        return status_ok(last_status_);
    }

    uint64_t CoSimulationModel::step_until(uint64_t stop_time)
    {
        auto sim_time = get_simulation_time();
        while (sim_time < stop_time)
        {
            auto step_time = stop_time - sim_time;

            IF_LOG({
                log(debug)("[{}] step_time {}s ", __func__, utils::time::ns_to_s(step_time));
            });

            if (!this->step(step_time))
            {
                int status = last_status();
                log(error)("Error! step() returned with status: {}", std::to_string(status));
                if (status == 3)
                {
                    throw std::runtime_error("Execution failed");
                }
            }
            sim_time = get_simulation_time();

            IF_LOG({
                log(trace)("[{}], sim time {}", __func__, sim_time);
            });
        }
        return sim_time;
    }

    bool CoSimulationModel::step(uint64_t step_size)
    {
        if (!instantiated_)
        {
            throw std::logic_error("step called before instantiate");
        }

        double current = utils::time::ns_to_s(current_time_);
        double step_value = utils::time::ns_to_s(step_size);

        IF_LOG({
            log(debug)("[{}] current {} step {}", __func__, current, step_value);
        });

        last_status_ = fmi2_doStep(handle, current, step_value, fmi2True);
        if (status_ok(last_status_))
        {
            current_time_ += step_size;
            return true;
        }
        return false;
    }

    bool CoSimulationModel::terminate()
    {
        if (!instantiated_)
        {
            return true;
        }

        log(debug)("[{}] Terminating FMU {}", __func__, instance_.path());
        last_status_ = fmi2_terminate(handle);
        fmi2_freeInstance(handle);
        instantiated_ = false;
        return status_ok(last_status_);
    }

    uint64_t CoSimulationModel::get_simulation_time() const
    {
        return current_time_;
    }

    fmi2Status CoSimulationModel::last_status() const
    {
        return last_status_;
    }

    bool CoSimulationModel::set_real_input_derivative(uint64_t value_reference, int derivative_order, double value)
    {
        fmi2ValueReference vr = static_cast<fmi2ValueReference>(value_reference);
        fmi2Integer order = static_cast<fmi2Integer>(derivative_order);
        last_status_ = fmi2_setRealInputDerivatives(handle, &vr, 1, &order, &value);
        return status_ok(last_status_);
    }

    bool CoSimulationModel::get_real_output_derivative(uint64_t value_reference, int derivative_order, double &out)
    {
        fmi2ValueReference vr = static_cast<fmi2ValueReference>(value_reference);
        fmi2Integer order = static_cast<fmi2Integer>(derivative_order);
        last_status_ = fmi2_getRealOutputDerivatives(handle, &vr, 1, &order, &out);
        return status_ok(last_status_);
    }

    bool CoSimulationModel::read_real(uint64_t value_reference, double &out)
    {
        fmi2ValueReference vr = static_cast<fmi2ValueReference>(value_reference);
        last_status_ = fmi2_getReal(handle, &vr, 1, &out);
        return status_ok(last_status_);
    }

    bool CoSimulationModel::read_integer(uint64_t value_reference, int &out)
    {
        fmi2ValueReference vr = static_cast<fmi2ValueReference>(value_reference);
        last_status_ = fmi2_getInteger(handle, &vr, 1, &out);
        return status_ok(last_status_);
    }

    bool CoSimulationModel::read_boolean(uint64_t value_reference, int &out)
    {
        fmi2ValueReference vr = static_cast<fmi2ValueReference>(value_reference);
        last_status_ = fmi2_getBoolean(handle, &vr, 1, &out);
        return status_ok(last_status_);
    }

    bool CoSimulationModel::read_string(uint64_t value_reference, std::string &out)
    {
        fmi2ValueReference vr = static_cast<fmi2ValueReference>(value_reference);
        fmi2String value = nullptr;
        last_status_ = fmi2_getString(handle, &vr, 1, &value);
        if (status_ok(last_status_))
        {
            out = std::string(value);
            return true;
        }
        return false;
    }

    bool CoSimulationModel::write_real(uint64_t value_reference, double value)
    {
        fmi2ValueReference vr = static_cast<fmi2ValueReference>(value_reference);
        last_status_ = fmi2_setReal(handle, &vr, 1, &value);

        // There must be a read after the last write for some fmus...
        // Or there is a solver restart
        // can be seen as an event
        fmi2Real data = 0.0;
        fmi2_getReal(handle, &vr, 1, &data);
        return status_ok(last_status_);
    }

    bool CoSimulationModel::write_integer(uint64_t value_reference, int value)
    {
        fmi2ValueReference vr = static_cast<fmi2ValueReference>(value_reference);
        last_status_ = fmi2_setInteger(handle, &vr, 1, &value);
        return status_ok(last_status_);
    }

    bool CoSimulationModel::write_boolean(uint64_t value_reference, int value)
    {
        fmi2ValueReference vr = static_cast<fmi2ValueReference>(value_reference);
        last_status_ = fmi2_setBoolean(handle, &vr, 1, &value);
        return status_ok(last_status_);
    }

    bool CoSimulationModel::write_string(uint64_t value_reference, const std::string &value)
    {
        fmi2ValueReference vr = static_cast<fmi2ValueReference>(value_reference);
        fmi2String data = value.c_str();
        last_status_ = fmi2_setString(handle, &vr, 1, &data);
        return status_ok(last_status_);
    }

}
