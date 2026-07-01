#include "analysis/components/analysis_connector.hpp"

#include "FMI2_Enums_Ext.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <sstream>
#include <utility>

namespace ssp4sim::analysis
{
    namespace
    {
        ssp4cpp::utils::log::Logger *model_log()
        {
            static ssp4cpp::utils::log::Logger *logger =
                ssp4cpp::utils::log::make_logger("ssp4sim.analysis.SspConnector");
            return logger;
        }
    }

    SspConnector::SspConnector(std::string connector_name_,
                                         uint64_t value_reference_,
                                         types::DataType data_type_,
                                         types::Causality causality_);
        : name(connector_name_)
        {
            type = SspItemType::Connector;
            value_reference = value_reference_;
            data_type = data_type_;
            causality = causality_;

            initial_value = ext::ParameterValue(name, data_type);
        }

        std::string SspConnector::to_string() const
        {
            std::ostringstream oss;
            oss << "Connector {"
                << "\n  name: " << name
                << "\n  vr: " << value_reference
                << "\n  type: " << data_type.to_string()
                << "\n  causality: " << causality.to_string()
                << "\n}";
            return oss.str();
        }

} // namespace ssp4sim::analysis