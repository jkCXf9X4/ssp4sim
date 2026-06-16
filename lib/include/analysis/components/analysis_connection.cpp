#include "analysis/components/analysis_connection.hpp"
#include "analysis/components/analysis_connector.hpp"

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
                ssp4cpp::utils::log::make_logger("ssp4sim.analysis.AnalysisConnection");
            return logger;
        }
    }

    AnalysisConnection::AnalysisConnection(std::string source_model_,
                                           std::string source_connector_,
                                           std::string target_model_,
                                           std::string target_connector_,
                                           uint64_t delay_,
                                           bool is_boundary_crossing_)
        : source_model(source_model_),
          source_connector(source_connector_),
          target_model(target_model_),
          target_connector(target_connector_),
          is_boundary_crossing(is_boundary_crossing_),
          delay(delay_)
    {
        // name is not used in this context, only for pretty prints
        name = source_model + "." + source_connector + "->" + target_model + "." + target_connector;
    }

    std::string AnalysisConnection::to_string() const
    {
        std::ostringstream oss;
        oss << "Connection {"
            << "\n  source: " << source_model << "." << source_connector
            << "\n  target: " << target_model << "." << target_connector
            << "\n  delay: " << delay
            << "\n}";
        return oss.str();
    }

    std::string AnalysisConnection::create_name(const std::string &src_model,
                                                 const std::string &src_conn,
                                                 const std::string &tgt_model,
                                                 const std::string &tgt_conn)
    {
        return src_model + "." + src_conn + "->" + tgt_model + "." + tgt_conn;
    }

} // namespace ssp4sim::analysis