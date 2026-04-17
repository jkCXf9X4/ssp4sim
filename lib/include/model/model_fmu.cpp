#include "model/model_fmu.hpp"

#include "config.hpp"
#include "signal/storage.hpp"
#include "handler/fmu_handler.hpp"
#include "model/model_connection.hpp"
#include "model/model_connector.hpp"
#include "utils/time.hpp"
#include "utils/timer.hpp"

#include <memory>
#include <format>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace ssp4sim::graph
{

    FmuModel::FmuModel(std::string name, ssp4sim::handler::FmuInfo *fmu, size_t maxOutputDerivativeOrder)
        : log(ssp4cpp::utils::log::make_logger(std::format("models.{}", name)))
    {
        this->fmu = fmu;
        this->name = std::move(name);
        this->maxOutputDerivativeOrder = maxOutputDerivativeOrder;
        
        input_area = std::make_unique<ssp4sim::signal::SignalStorage>(10, this->name + ".input");
        output_area = std::make_unique<ssp4sim::signal::SignalStorage>(200, this->name + ".output");
        forward_derivatives = utils::Config::getOr("simulation.executor.forward_derivatives", true);
        fmu_logging = utils::Config::getOr("simulation.log.fmu", false);
    }

    FmuModel::~FmuModel()
    {
        LOG_TRACE_L1(log, "[{}] Destroying FmuModel", __func__);
        if (fmu != nullptr && fmu->model != nullptr)
        {
            fmu->model->terminate();
        }
    }

    std::string FmuModel::to_string() const
    {
        std::ostringstream oss;
        oss << "FmuModel { \n"
            << "Name: " << name
            << "\n}\n";
        return oss.str();
    }

    void FmuModel::enter_init()
    {
        LOG_TRACE_L1(log, "[{}] FmuModel init {}", __func__, name);
        fmu->model->instantiate(false, fmu_logging); // visible, logging on

        LOG_TRACE_L1(log, "[{}] Input area: {}", __func__, input_area->to_string());
        LOG_TRACE_L1(log, "[{}] Output area: {}", __func__, output_area->to_string());

        double start_time = utils::Config::getDouble("simulation.start_time");
        double timestep = utils::Config::getDouble("simulation.timestep");
        double end_time = utils::Config::getDouble("simulation.stop_time");
        double tolerance = utils::Config::getDouble("simulation.tolerance");

        LOG_DEBUG(log, "[{}] setup_experiment: {}", __func__, name);

        // The simulation may take one step beyond the stop_time. Some fmus may crash due to this
        // therfore tell the fmus that stop + one step should be ok
        if (!fmu->model->setup_experiment(utils::time::s_to_ns(start_time), utils::time::s_to_ns(end_time + timestep * 10), tolerance))
        {
            LOG_ERROR(log, "[{}] setup_experiment failed for {}, this may be due to a stop time that is larger than the DefaultExperiment specifed in the fmus. ", __func__, name);
            throw std::runtime_error(std::format("[{}] setup_experiment failed for {}", __func__, name));
        }

        LOG_DEBUG(log, "[{}] enter_initialization_mode: {}", __func__, name);
        if (!fmu->model->enter_initialization_mode())
        {
            LOG_ERROR(log, "[{}] enter_initialization_mode failed for {}", __func__, name);
            throw std::runtime_error(std::format("[{}] enter_initialization_mode failed for {}", __func__, name));
        }

        LOG_TRACE_L1(log, "[{}] Set input area", __func__);
        ConnectorInfo::set_initial_input_area(this->input_area.get(), this->inputs, 0);

        LOG_TRACE_L1(log, "[{}] Set start values", __func__);
        ConnectorInfo::set_start_values(this->parameters);
        ConnectorInfo::set_start_values(this->inputs);
    }

    void FmuModel::exit_init()
    {
        LOG_TRACE_L1(log, "[{}] FmuModel init {}", __func__, name);
        LOG_DEBUG(log, "[{}] exit_initialization_mode: {}", __func__, name);
        if (!fmu->model->exit_initialization_mode())
        {
            LOG_ERROR(log, "[{}] exit_initialization_mode failed for {}", __func__, name);
            throw std::runtime_error(std::format("[{}] exit_initialization_mode failed for {}", __func__, name));
        }

        LOG_TRACE_L1(log, "[{}] FmuModel init completed", __func__);
    }

    uint64_t FmuModel::direct_feedthrough(uint64_t start)
    {
        LOG_WARNING(log, "[{}] This solution is not ok. Doing direct feedthrough for all variables will overwrite inputs with outputs that are unset. It can only be done for the relevant algebraic loops. Nothing else!", __func__);

        auto target_area = input_area->get_or_push(start);

        IF_LOG({
            LOG_INFO(log, "[{}] Propagating at start_time {}, input_area {} timestamp {}", __func__, start, target_area, input_area->data->timestamps[target_area]);
        });

        ConnectionInfo::retrieve_model_inputs(connections, target_area, start);

        ConnectorInfo::write_data_to_model(inputs, input_area.get(), target_area);

        auto area = output_area->get_or_push(start);

        IF_LOG({
            LOG_INFO(log, "[{}] Propagating at start_time {}, output area {} timestamp {}", __func__, start, area, output_area->data->timestamps[area]);
        });

        ConnectorInfo::read_values_from_model(outputs, output_area.get(), area);
        return start;
    }

    void FmuModel::pre(uint64_t input_time)
    {
        IF_LOG({
            LOG_TRACE_L1(log, "[{}] Init. current_time {}, input_time {}", __func__, current_time, input_time);
        });

        auto target_area = input_area->push(input_time);

        ConnectionInfo::retrieve_model_inputs(connections, target_area, input_time);

        input_area->flag_new_data(target_area);

        ConnectorInfo::write_data_to_model(inputs, input_area.get(), target_area);

        if (forward_derivatives)
        {
            auto model_timer = utils::time::Timer();
            ConnectorInfo::apply_input_derivatives(inputs, target_area);
            this->walltime_ns += model_timer.stop();
        }

        IF_LOG({
            LOG_TRACE_L1(log, "[{}] Input area after pre: {}", __func__, input_area->export_area(target_area));
        });
    }

    void FmuModel::post(uint64_t time)
    {
        IF_LOG({
            LOG_TRACE_L1(log, "[{}] Store results, timestamp: {}", __func__, time);
        });

        auto area = output_area->push(time);

        ConnectorInfo::read_values_from_model(outputs, output_area.get(), area);

        if (forward_derivatives && current_time != 0)
        {
            auto model_timer = utils::time::Timer();
            ConnectorInfo::fetch_output_derivatives(outputs, area);
            this->walltime_ns += model_timer.stop();
        }
        output_area->flag_new_data(area);

        IF_LOG({
            LOG_TRACE_L1(log, "[{}] Output area after post: {}", __func__, output_area->export_area(area));
        });
    }

    uint64_t FmuModel::step(StepData step_data)
    {
        IF_LOG({
            LOG_DEBUG(log, "[{}] Init {}, current_time {}, stepdata: {}", __func__, name, current_time, step_data.to_string());
        });

        pre(step_data.input_time);

        IF_LOG({
            LOG_DEBUG(log, "[{}] Step until {}", __func__, step_data.end_time);
        });

        auto model_timer = utils::time::Timer();
        current_time = fmu->model->step_until(step_data.end_time);
        this->walltime_ns += model_timer.stop();

        post(step_data.output_time);

        IF_LOG({
            LOG_TRACE_L1(log, "[{}] Completed, current_time:", __func__, current_time);
        });

        return current_time;
    }

    uint64_t FmuModel::invoke(StepData step_data)
    {
        return step(step_data);
    }

}
