#include "model_connection.hpp"


#include "../../1_ssp_parser/schema_extensions/FMI2_Enums_Ext.hpp"
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
            << ", mode: " << static_cast<int>(mode)
            << ", time_offset: " << time_offset
            << ", forward_derivatives: " << forward_derivatives_order
            << ", is_feedthrough: " << (is_feedthrough ? "true" : "false")
            << " }";
        return oss.str();
    }

    void ConnectionInfo::retrieve_model_inputs(std::vector<ConnectionInfo> &connections,
                                               int target_area,
                                               uint64_t input_time,
                                               uint64_t step_start,
                                               uint64_t step_end)
    {
        for (auto &connection : connections)
        {
            IF_LOG({
                LOG_TRACE_L2(connection.log, "[{func}] Area {}", __func__, target_area);
                LOG_TRACE_L1(connection.log, "[{func}] Copy connections", __func__);
                LOG_TRACE_L2(connection.log, "[{func}] Fetch valid data connection {}", __func__, connection.to_string());
            });

            int64_t reference;
            switch (connection.mode)
            {
                case DataAccessMode::StartTime:
                    reference = static_cast<int64_t>(step_start);
                    break;
                case DataAccessMode::EndTime:
                    reference = static_cast<int64_t>(step_end);
                    break;
                case DataAccessMode::LatestTime:
                default:
                    reference = static_cast<int64_t>(input_time);
                    break;
            }
            reference += connection.time_offset;

            int64_t lookup_time = reference - static_cast<int64_t>(connection.delay);

            size_t source_area;
            bool found = lookup_time >= 0 &&
                         connection.source_storage->find_latest_valid_area(static_cast<std::uint64_t>(lookup_time), source_area);

            if (found)
            {
                IF_LOG({
                    LOG_DEBUG(connection.log, "[{func}] Valid source_storage area found, time {time}", __func__, connection.source_storage->ring->timestamps[source_area]);
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
                    LOG_WARNING(connection.log, "[{func}] No valid data at lookup time {time}, connection: {connection}", __func__, lookup_time, connection.to_string());
                }
            }
        }
    }

}
