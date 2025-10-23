#pragma once

#include "utils/node.hpp"
#include "utils/map.hpp"
#include "utils/time.hpp"

#include "ssp4sim_definitions.hpp"

#include "FMI2_Enums_Ext.hpp"

#include "data_ring_storage.hpp"
#include "data_recorder.hpp"
#include "invocable.hpp"
#include "config.hpp"

#include "fmu_handler.hpp"

#include "ssp4cpp/utils/interface.hpp"

#include <string>
#include <vector>
#include <functional>

namespace ssp4sim::graph
{
    struct ConnectionInfo : public ssp4cpp::utils::interfaces::IString
    {
        static inline Logger log = Logger("ssp4sim.model.ConnectionInfo", LogLevel::info);

        utils::DataType type;
        size_t size;

        utils::RingStorage *source_storage;
        utils::RingStorage *target_storage;
        uint32_t source_index;
        uint32_t target_index;

        bool forward_derivatives = false;
        int forward_derivatives_order = 0;

        virtual void print(std::ostream &os) const
        {
            os << "ConnectionInfo { "
               << "type: " << type
               << ", size: " << size
               << ", source_storage: " << source_storage
               << ", target_storage: " << target_storage
               << ", source_index: " << source_index
               << ", target_index: " << target_index
               << ", forward_derivatives: " << forward_derivatives_order
               << " }";
        }

        static inline void retrieve_model_inputs(std::vector<ConnectionInfo> &connections, int target_area, uint64_t valid_input_time)
        {
            IF_LOG({
                log(ext_trace)("[{}] Area {}", __func__, target_area);
                log(trace)("[{}] Copy connections", __func__);
            });

            for (auto &connection : connections)
            {
                IF_LOG({
                    log(ext_trace)("[{}] Fetch valid data connection {}", __func__, connection.to_string());
                });

                auto source_area = connection.source_storage->get_valid_area(valid_input_time);

                if (source_area != -1)
                {
                    IF_LOG({
                        log(debug)("[{}] Valid source_storage area found, time {}", __func__, connection.source_storage->data->timestamps[source_area]);
                    });
                    
                    auto source_item = connection.source_storage->get_item(source_area, connection.source_index);
                    IF_LOG({
                        auto data_type_str = ssp4sim::ext::fmi2::enums::data_type_to_string(connection.type, source_item);
                        log(trace)("[{}] Found valid item, copying data to target area: {}",
                                    __func__, data_type_str);
                    });

                    auto target_item = connection.target_storage->get_item(target_area, connection.target_index);
                    memcpy(target_item, source_item, connection.size);

                    if (connection.forward_derivatives && connection.forward_derivatives)
                    {
                        IF_LOG({
                            log(ext_trace)("[{}] Copying derivatives {}", __func__, connection.to_string());
                        });

                        for (int order = 1; order <= connection.forward_derivatives_order; ++order)
                        {
                            auto source_der = connection.source_storage->get_derivative(source_area, connection.source_index, order);
                            auto target_der = connection.target_storage->get_derivative(target_area, connection.target_index, order);
                            IF_LOG({
                                log(ext_trace)("[{}] Copying derivatives {} -> {}", __func__, (uint64_t)source_der, (uint64_t)target_der);
                            });

                            memcpy(target_der, source_der, sizeof(double));
                        }
                    }
                }
                else
                {
                    if (valid_input_time > 1)
                    {
                        log(warning)("[{}] No valid data for t {}, connection", __func__, valid_input_time, connection.to_string());
                    }
                }
            }
        }
    };
}
