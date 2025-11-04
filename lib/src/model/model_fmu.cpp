#include "model/model_fmu.hpp"

#include "config.hpp"
#include "data_recorder.hpp"
#include "data_ring_storage.hpp"
#include "handler/fmu_handler.hpp"
#include "model/model_connection.hpp"
#include "model/model_connector.hpp"
#include "utils/time.hpp"
#include "utils/timer.hpp"

#include <format>
#include <memory>
#include <stdexcept>
#include <utility>

namespace ssp4sim::graph
{

    FmuModel::FmuModel(std::string name, ssp4sim::handler::FmuInfo *fmu)
        : log(std::format("models.{}", name), LogLevel::info)
    {
        this->fmu = fmu;
        this->name = std::move(name);
        input_area = std::make_unique<ssp4sim::utils::RingStorage>(10, this->name + ".input");
        output_area = std::make_unique<ssp4sim::utils::RingStorage>(40, this->name + ".output");
        forward_derivatives = utils::Config::getOr<bool>("simulation.executor.forward_derivatives", true);
    }

    FmuModel::~FmuModel()
    {
        log(ext_trace)("[{}] Destroying FmuModel", __func__);
        if (fmu != nullptr && fmu->model != nullptr)
        {
            fmu->model->terminate();
        }
    }

    void FmuModel::print(std::ostream &os) const
    {
        os << "FmuModel { \n"
           << "Name: " << name
           << "\n}\n";
    }

    void FmuModel::enter_init()
    {
        log(trace)("[{}] FmuModel init {}", __func__, name);

        log(trace)("[{}] Input area: {}", __func__, input_area->to_string());
        log(trace)("[{}] Output area: {}", __func__, output_area->to_string());

        double start_time = utils::Config::get<double>("simulation.start_time");
        double end_time = utils::Config::get<double>("simulation.stop_time") + 10;
        double tolerance = utils::Config::get<double>("simulation.tolerance");

        log(debug)("[{}] setup_experiment: {}", __func__, name);
        if (!fmu->model->setup_experiment(utils::time::s_to_ns(start_time), utils::time::s_to_ns(end_time), tolerance))
        {
            log(error)("[{}] setup_experiment failed ", __func__);
        }

        log(debug)("[{}] enter_initialization_mode: {}", __func__, name);
        if (!fmu->model->enter_initialization_mode())
        {
            log(error)("[{}] enter_initialization_mode failed ", __func__);
            throw std::runtime_error(std::format("[{}] enter_initialization_mode failed for {}", __func__, name));
        }

        log(ext_trace)("[{}] Set input area", __func__);
        ConnectorInfo::set_initial_input_area(this->input_area.get(), this->inputs, 0);

        log(trace)("[{}] Set start values", __func__);
        ConnectorInfo::set_start_values(this->parameters);
        ConnectorInfo::set_start_values(this->inputs);
    }

    void FmuModel::exit_init()
    {
        log(trace)("[{}] FmuModel init {}", __func__, name);
        log(debug)("[{}] exit_initialization_mode: {}", __func__, name);
        if (!fmu->model->exit_initialization_mode())
        {
            log(error)("[{}] exit_initialization_mode failed ", __func__);
            throw std::runtime_error(std::format("[{}] exit_initialization_mode failed for {}", __func__, name));
        }

        log(ext_trace)("[{}] FmuModel init completed", __func__);
    }

    uint64_t FmuModel::direct_feedthrough(uint64_t start)
    {
        log(warning)("[{}] This solution is not ok. Doing direct feedthrough for all variables will overwrite inputs with outputs that are unset. It can only be done for the relevant algebraic loops. Nothing else!", __func__);

        auto target_area = input_area->get_or_push(start);

        IF_LOG({
            log(info)("[{}] Propagating at start_time {}, input_area {} timestamp {}", __func__, start, target_area, input_area->data->timestamps[target_area]);
        });

        ConnectionInfo::retrieve_model_inputs(connections, target_area, start);

        ConnectorInfo::write_data_to_model(inputs, input_area.get(), target_area);

        auto area = output_area->get_or_push(start);

        IF_LOG({
            log(info)("[{}] Propagating at start_time {}, output area {} timestamp {}", __func__, start, area, output_area->data->timestamps[area]);
        });

        ConnectorInfo::read_values_from_model(outputs, output_area.get(), area);
        return start;
    }

    void FmuModel::pre(uint64_t input_time)
    {
        IF_LOG({
            log(trace)("[{}] Init. current_time {}, input_time {}", __func__, current_time, input_time);
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
            log(trace)("[{}] Input area after pre: {}", __func__, input_area->data->export_area(target_area));
        });
    }

    void FmuModel::post(uint64_t time)
    {
        IF_LOG({
            log(trace)("[{}] Store results, timestamp: {}", __func__, time);
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
            log(trace)("[{}] Output area after post: {}", __func__, output_area->data->export_area(area));
        });
    }

    uint64_t FmuModel::step(StepData step_data)
    {
        IF_LOG({
            log(debug)("[{}] Init {}, current_time {}, stepdata: {}", __func__, name, current_time, step_data.to_string());
        });

        pre(step_data.input_time);

        IF_LOG({
            log(debug)("[{}] Step until {}", __func__, step_data.end_time);
        });

        auto model_timer = utils::time::Timer();
        current_time = fmu->model->step_until(step_data.end_time);
        this->walltime_ns += model_timer.stop();

        post(step_data.output_time);

        IF_LOG({
            log(ext_trace)("[{}] Completed, current_time:", __func__, current_time);
        });

        return current_time;
    }

    uint64_t FmuModel::invoke(StepData step_data)
    {
        return step(step_data);
    }

}
