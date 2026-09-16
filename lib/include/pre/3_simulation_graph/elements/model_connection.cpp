#include "model_connection.hpp"


#include "../../1_ssp_parser/schema_extensions/FMI2_Enums_Ext.hpp"
#include "signal/storage.hpp"

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

}