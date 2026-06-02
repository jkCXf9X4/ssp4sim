#include "model/model_connection.hpp"


#include "FMI2_Enums_Ext.hpp"
#include "signal/storage.hpp"

#include <cstring>
#include <sstream>
#include <vector>

namespace ssp4sim::graph
{

    std::string ConnectionInfo::to_string() const
    {
        std::ostringstream oss;
        oss << "ConnectionInfo { "
            << "type: " << type
            << ", size: " << size
            << ", source_storage: " << source_storage->name
            << ", target_storage: " << target_storage->name
            << ", source_index: " << source_index
            << ", target_index: " << target_index
            << ", forward_derivatives: " << forward_derivatives_order
            << ", is_feedthrough: " << (is_feedthrough ? "true" : "false")
            << " }";
        return oss.str();
    }

    void ConnectionInfo::retrieve_model_inputs(std::vector<ConnectionInfo> &connections,
                                               int target_area,
                                               uint64_t input_time)
    {
        for (auto &connection : connections)
        {
            IF_LOG({
                LOG_TRACE_L2(connection.log, "[{func}] Area {}", __func__, target_area);
                LOG_TRACE_L1(connection.log, "[{func}] Copy connections", __func__);
                LOG_TRACE_L2(connection.log, "[{func}] Fetch valid data connection {}", __func__, connection.to_string());
            });

            size_t source_area;
            if (connection.source_storage->find_latest_valid_area(input_time - connection.delay, source_area))
            {
                IF_LOG({
                    LOG_DEBUG(connection.log, "[{func}] Valid source_storage area found, time {time}", __func__, connection.source_storage->data->timestamps[source_area]);
                });

                auto source_item = connection.source_storage->get_item(static_cast<std::size_t>(source_area), connection.source_index);
                IF_LOG({
                    auto data_type_str = ssp4sim::ext::fmi2::enums::data_type_to_string(connection.type, source_item);
                    LOG_TRACE_L1(connection.log, "[{func}] Found valid item, copying data to target area: {}", __func__, data_type_str);
                });

                auto target_item = connection.target_storage->get_item(static_cast<std::size_t>(target_area), connection.target_index);
                std::memcpy(target_item, source_item, connection.size);

                if (connection.forward_derivatives)
                {
                    IF_LOG({
                        LOG_TRACE_L2(connection.log, "[{func}] Copying derivatives {}", __func__, connection.to_string());
                    });

                    for (int order = 1; order <= connection.forward_derivatives_order; ++order)
                    {
                        auto source_der = connection.source_storage->get_derivative(static_cast<std::size_t>(source_area),
                                                                                    connection.source_index,
                                                                                    order);
                        auto target_der = connection.target_storage->get_derivative(static_cast<std::size_t>(target_area),
                                                                                    connection.target_index,
                                                                                    order);
                        IF_LOG({
                            LOG_TRACE_L2(connection.log, "[{func}] Copying derivatives {} -> {}", __func__, reinterpret_cast<uint64_t>(source_der), reinterpret_cast<uint64_t>(target_der));
                        });

                        if (source_der != nullptr && target_der != nullptr)
                        {
                            std::memcpy(target_der, source_der, sizeof(double));
                        }
                    }
                }
            }
            else
            {
                if (input_time > 1)
                {
                    LOG_WARNING(connection.log, "[{func}] No valid data for t {time}, connection: {connection}", __func__, input_time, connection.to_string());
                }
            }
        }
    }

}
