#include "analysis/components/analysis_connection.hpp"

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
                ssp4cpp::utils::log::make_logger("ssp4sim.analysis.Connection");
            return logger;
        }
    }

    SspConnection::SspConnection(std::string source_model_,
                                           std::string source_connector_,
                                           std::string target_model_,
                                           std::string target_connector_)
        : source_model(source_model_), source_connector(source_connector_),
          target_model(target_model_), target_connector(target_connector_)
    {
        type = SspItemType::Connection;
        
        // name is not used in this context, only for pretty prints
        name = source_model + "." + source_connector + "->" + target_model + "." + target_connector;

        is_boundary = source_model == "" or target_model_ == "";
    }

    std::string SspConnection::to_string() const
    {
        std::ostringstream oss;
        oss << "Connection {"
            << "\n  name: " << name
            << "\n  delay: " << delay
            << "\n}";
        return oss.str();
    }

} // namespace ssp4sim::analysis