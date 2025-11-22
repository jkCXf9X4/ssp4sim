#include "model/model_connector.hpp"

#include "FMI2_Enums_Ext.hpp"

#include "handler/fmu_handler.hpp"
#include "signal/ring_storage.hpp"

namespace ssp4sim::graph
{

    Logger ConnectorInfo::log = Logger("ssp4sim.model.ConnectorInfo_s", LogLevel::info);

    void ConnectorInfo::print(std::ostream &os) const
    {
        os << "ConnectorInfo { "
           << "name: " << name
           << ", type: " << type
           << ", size: " << size
           << ", index: " << index
           << ", value_ref: " << value_ref
           << ", forward_derivatives: " << forward_derivatives_order
           << " }";
    }

    void ConnectorInfo::set_start_values(std::map<std::string, ConnectorInfo> &connectors)
    {
        for (auto &[name, connector] : connectors)
        {
            if (!connector.initial_value)
            {
                continue;
            }

            auto data_ptr = static_cast<void *>(connector.initial_value.get());
            auto data_type_str = ssp4sim::ext::fmi2::enums::data_type_to_string(connector.type, data_ptr);
            log(debug)("[{}] Set initial value for {}, {} : {}", __func__, name, connector.type.to_string(), data_type_str);

            utils::write_to_model_(connector.type, *connector.fmu->model, connector.value_ref, data_ptr);
        }
    }

    void ConnectorInfo::set_initial_input_area(ssp4sim::signal::RingStorage *input_area,
                                               std::map<std::string, ConnectorInfo> &inputs,
                                               uint64_t time)
    {
        log(trace)("[{}] Set input start area", __func__);
        auto area = input_area->push(time);

        for (auto &[name, input] : inputs)
        {
            if (!input.initial_value)
            {
                continue;
            }

            auto data_ptr = static_cast<void *>(input.initial_value.get());
            auto item = input_area->get_item(area, input.index);

            auto data_type_str = ssp4sim::ext::fmi2::enums::data_type_to_string(input.type, data_ptr);
            log(debug)("[{}] Set initial input value for {}, {} : {}", __func__, name, input.type.to_string(), data_type_str);

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

        log(trace)("[{}] Input area after initialization: {}", __func__, input_area->data->export_area(area));
    }

    void ConnectorInfo::write_data_to_model(std::map<std::string, ConnectorInfo> &inputs,
                                            ssp4sim::signal::RingStorage *storage,
                                            int area)
    {
        IF_LOG({
            log(debug)("[{}] Write data to model, time: {}", __func__, storage->data->timestamps[area]);
        });

        for (auto &[_, input] : inputs)
        {
            auto input_item = storage->get_item(area, input.index);

            IF_LOG({
                auto data_type_str = ssp4sim::ext::fmi2::enums::data_type_to_string(input.type, input_item);
                log(debug)("[{}] Copying input to model. {}, data: {}", __func__, input.to_string(), data_type_str);
            });

            utils::write_to_model_(input.type, *input.fmu->model, input.value_ref, static_cast<void *>(input_item));
        }
    }

    void ConnectorInfo::read_values_from_model(std::map<std::string, ConnectorInfo> &outputs,
                                               ssp4sim::signal::RingStorage *storage,
                                               int area)
    {
        IF_LOG({
            log(debug)("[{}] Init, area {}, time {}", __func__, area, storage->data->timestamps[area]);
        });

        for (auto &[_, output] : outputs)
        {
            auto item = storage->get_item(area, output.index);
            IF_LOG({
                log(ext_trace)("[{}] Copying ref {} ({}) to index {}", __func__, output.value_ref, output.type.to_string(), output.index);
            });

            utils::read_from_model_(output.type, *output.fmu->model, output.value_ref, static_cast<void *>(item));

            IF_LOG({
                auto data_type_str = ssp4sim::ext::fmi2::enums::data_type_to_string(output.type, item);
                log(debug)("[{}] Copying output from model. {}, data: {}", __func__, output.to_string(), data_type_str);
            });
        }
        IF_LOG({
            log(ext_trace)("[{}] Completed copy from model", __func__);
        });
    }

    void ConnectorInfo::apply_input_derivatives(std::map<std::string, ConnectorInfo> &inputs,
                                                std::size_t area)
    {
        IF_LOG({
            log(trace)("[{}] Init area {}", __func__, area);
        });

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
                    log(warning)("[{}] Failed to set input derivative order {} for {} (status {})",
                                 __func__,
                                 order,
                                 connector.name,
                                 static_cast<int>(connector.fmu->model->last_status()));
                }
            }
        }
    }

    void ConnectorInfo::fetch_output_derivatives(std::map<std::string, ConnectorInfo> &outputs,
                                                 std::size_t area)
    {
        IF_LOG({
            log(trace)("[{}] Init area {}", __func__, area);
        });

        for (auto &[_, connector] : outputs)
        {
            if (!connector.forward_derivatives)
            {
                continue;
            }

            for (int order = 1; order <= connector.forward_derivatives_order; ++order)
            {
                IF_LOG({
                    log(trace)("[{}] get_derivative position for vr:{} name: {} order: {}",
                               __func__,
                               connector.value_ref,
                               connector.name,
                               order);
                });

                auto der_ptr = connector.storage->get_derivative(area, connector.index, order);
                if (der_ptr == nullptr)
                {
                    log(warning)("[{}] Failed to find derivative item for {}", __func__, connector.name);
                    continue;
                }

                double value = 0.0;
                if (!connector.fmu->model->get_real_output_derivative(connector.value_ref, order, value))
                {
                    log(warning)("[{}] Failed to get output derivative order {} for {} (status {})",
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
