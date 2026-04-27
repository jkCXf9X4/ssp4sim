#include "model/model_connector.hpp"

#include "FMI2_Enums_Ext.hpp"

#include <cstring>
#include <sstream>

namespace ssp4sim::graph
{
    std::string ConnectorInfo::to_string() const
    {
        std::ostringstream oss;
        oss << "ConnectorInfo { "
            << "name: " << name
            << ", type: " << type
            << ", size: " << size
            << ", index: " << index
            << ", value_ref: " << value_ref
            << ", forward_derivatives: " << forward_derivatives_order
            << " }";
        return oss.str();
    }

    void ConnectorInfo::set_start_values(std::unordered_map<std::string, ConnectorInfo> &connectors)
    {
        for (auto &[name, connector] : connectors)
        {
            if (!connector.initial_value)
            {
                continue;
            }

            auto data_ptr = connector.initial_value->raw_ptr();
            auto data_type_str = ssp4sim::ext::fmi2::enums::data_type_to_string(connector.type, data_ptr);
            LOG_DEBUG(connector.log, "[{func}] Set initial value for {name}, {type} : {data_type}", __func__, name, connector.type.to_string(), data_type_str);

            utils::write_to_model_(connector.type, *connector.fmu->model, connector.value_ref, data_ptr);
        }
    }

    void ConnectorInfo::set_initial_input_area(ssp4sim::signal::SignalStorage *input_area,
                                               std::unordered_map<std::string, ConnectorInfo> &inputs,
                                               uint64_t time)
    {
        LOG_TRACE_L1(input_area->log, "[{func}] Set input start area", __func__);
        auto area = input_area->push(time);

        for (auto &[name, input] : inputs)
        {
            if (!input.initial_value)
            {
                continue;
            }

            auto data_ptr = input.initial_value->raw_ptr();
            auto item = input_area->get_item(area, input.index);

            auto data_type_str = ssp4sim::ext::fmi2::enums::data_type_to_string(input.type, data_ptr);
            LOG_DEBUG(input_area->log, "[{func}] Set initial input value for {name}, {type} : {data_type}", __func__, name, input.type.to_string(), data_type_str);

            if (input.type == types::DataType::string)
            {
                *(std::string *)item = *(std::string *)data_ptr;
            }
            else
            {
                std::memcpy(item, data_ptr, input.size);
            }
        }
        input_area->flag_new_data(area);

        LOG_TRACE_L1(input_area->log, "[{func}] Input area after initialization: {}", __func__, input_area->export_area(area));
    }

    void ConnectorInfo::write_data_to_model(std::unordered_map<std::string, ConnectorInfo> &inputs,
                                            ssp4sim::signal::SignalStorage *storage,
                                            int area)
    {
        IF_LOG({
            LOG_DEBUG(storage->log, "[{func}] Write data to model, time: {time}", __func__, storage->data->timestamps[area]);
        });

        for (auto &[_, input] : inputs)
        {
            auto input_item = storage->get_item(area, input.index);

            IF_LOG({
                auto data_type_str = ssp4sim::ext::fmi2::enums::data_type_to_string(input.type, input_item);
                LOG_DEBUG(storage->log, "[{func}] Copying input to model. {input}, data: {data}", __func__, input.to_string(), data_type_str);
            });

            utils::write_to_model_(input.type, *input.fmu->model, input.value_ref, static_cast<void *>(input_item));
        }
    }

    void ConnectorInfo::read_values_from_model(std::unordered_map<std::string, ConnectorInfo> &outputs,
                                               ssp4sim::signal::SignalStorage *storage,
                                               int area)
    {
        IF_LOG({
            LOG_DEBUG(storage->log, "[{func}] Init, area {area}, time {time}", __func__, area, storage->data->timestamps[area]);
        });

        for (auto &[_, output] : outputs)
        {
            auto item = storage->get_item(area, output.index);
            IF_LOG({
                LOG_TRACE_L1(storage->log, "[{func}] Copying ref {} ({}) to index {}", __func__, output.value_ref, output.type.to_string(), output.index);
            });

            utils::read_from_model_(output.type, *output.fmu->model, output.value_ref, static_cast<void *>(item));

            IF_LOG({
                auto data_type_str = ssp4sim::ext::fmi2::enums::data_type_to_string(output.type, item);
                LOG_DEBUG(storage->log, "[{func}] Copying output from model. {output}, data: {data}", __func__, output.to_string(), data_type_str);
            });
        }
        IF_LOG({
            LOG_TRACE_L1(storage->log, "[{func}] Completed copy from model", __func__);
        });
    }

    void ConnectorInfo::apply_input_derivatives(std::unordered_map<std::string, ConnectorInfo> &inputs,
                                                std::size_t area)
    {

        for (auto &[_, connector] : inputs)
        {
            if (!connector.forward_derivatives)
            {
                continue;
            }

            for (int order = 1; order <= connector.forward_derivatives_order; ++order)
            {
                auto der_ptr = connector.storage->get_derivative(area, connector.index, order);
                if (der_ptr == nullptr)
                {
                    continue;
                }

                double value = *reinterpret_cast<double *>(der_ptr);
                if (!connector.fmu->model->set_real_input_derivative(connector.value_ref, order, value))
                {
                    LOG_WARNING(connector.log, "[{func}] Failed to set input derivative order {order} for {name} (status {status})",
                                 __func__,
                                 order,
                                 connector.name,
                                 static_cast<int>(connector.fmu->model->last_status()));
                }
            }
        }
    }

    void ConnectorInfo::fetch_output_derivatives(std::unordered_map<std::string, ConnectorInfo> &outputs,
                                                 std::size_t area)
    {
        for (auto &[_, connector] : outputs)
        {
            IF_LOG({
                LOG_TRACE_L1(connector.log, "[{func}] Init area {}", __func__, area);
            });

            if (!connector.forward_derivatives)
            {
                continue;
            }

            for (int order = 1; order <= connector.forward_derivatives_order; ++order)
            {
                IF_LOG({
                    LOG_TRACE_L1(connector.log, "[{func}] get_derivative position for vr:{} name: {} order: {}",
                               __func__,
                               connector.value_ref,
                               connector.name,
                               order);
                });

                auto der_ptr = connector.storage->get_derivative(area, connector.index, order);
                if (der_ptr == nullptr)
                {
                    LOG_WARNING(connector.log, "[{func}] Failed to find derivative item for {name}", __func__, connector.name);
                    continue;
                }

                double value = 0.0;
                if (!connector.fmu->model->get_real_output_derivative(connector.value_ref, order, value))
                {
                    LOG_WARNING(connector.log, "[{func}] Failed to get output derivative order {order} for {name} (status {status})",
                                 __func__,
                                 order,
                                 connector.name,
                                 static_cast<int>(connector.fmu->model->last_status()));
                    continue;
                }

                *reinterpret_cast<double *>(der_ptr) = value;
            }
        }
    }

}
