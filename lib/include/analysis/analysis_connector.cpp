#include "analysis/analysis_connector.hpp"

#include "FMI2_Enums_Ext.hpp"

#include <sstream>
#include <utility>

namespace ssp4sim::analysis
{

    AnalysisConnector::AnalysisConnector(std::string component_name,
                                          std::string connector_name_,
                                          uint64_t value_reference_,
                                          types::DataType data_type_)
        : name(create_name(component_name, connector_name_)),
          value_reference(value_reference_),
          data_type(data_type_),
          size(ext::fmi2::enums::get_data_type_size(data_type_))
    {
    }

    AnalysisConnector::~AnalysisConnector() = default;

    std::string AnalysisConnector::create_name(const std::string &component_name,
                                                 const std::string &connector_name_)
    {
        return component_name + "." + connector_name_;
    }

    std::string AnalysisConnector::to_string() const
    {
        std::ostringstream oss;
        oss << "Connector {"
            << "\n  name: " << name
            << "\n  vr: " << value_reference
            << "\n  type: " << static_cast<int>(data_type)
            << "\n  causality: " << static_cast<int>(causality)
            << "\n  is_boundary: " << is_boundary
            << "\n}";
        return oss.str();
    }

} // namespace ssp4sim::analysis