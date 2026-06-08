#include "analysis/analysis_connection.hpp"

#include <sstream>
#include <utility>

namespace ssp4sim::analysis
{

    AnalysisConnection::AnalysisConnection(std::string source_model_,
                                            std::string source_connector_,
                                            std::string target_model_,
                                            std::string target_connector_,
                                            uint64_t delay_,
                                            bool is_boundary_crossing_)
        : source_model(std::move(source_model_)),
          source_connector(std::move(source_connector_)),
          target_model(std::move(target_model_)),
          target_connector(std::move(target_connector_)),
          delay(delay_),
          is_boundary_crossing(is_boundary_crossing_)
    {
    }

    std::string AnalysisConnection::create_name(const std::string &src_model,
                                                  const std::string &src_con,
                                                  const std::string &tgt_model,
                                                  const std::string &tgt_con)
    {
        return src_model + "." + src_con + "->" + tgt_model + "." + tgt_con;
    }

    std::string AnalysisConnection::to_string() const
    {
        std::ostringstream oss;
        oss << "Connection {"
            << "\n  source: " << source_model << "." << source_connector
            << "\n  target: " << target_model << "." << target_connector
            << "\n  delay: " << delay
            << "\n  is_boundary_crossing: " << is_boundary_crossing
            << "\n}";
        return oss.str();
    }

} // namespace ssp4sim::analysis