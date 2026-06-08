#pragma once

#include <cstdint>
#include <string>

namespace ssp4sim::analysis
{

    class AnalysisConnection
    {
    public:
        std::string source_model;
        std::string source_connector;
        std::string target_model;
        std::string target_connector;
        uint64_t delay = 0;
        bool is_boundary_crossing = false;

        AnalysisConnection() = default;

        AnalysisConnection(std::string source_model_,
                           std::string source_connector_,
                           std::string target_model_,
                           std::string target_connector_,
                           uint64_t delay_ = 0,
                           bool is_boundary_crossing_ = false);

        ~AnalysisConnection() = default;

        static std::string create_name(const std::string &src_model,
                                        const std::string &src_con,
                                        const std::string &tgt_model,
                                        const std::string &tgt_con);

        std::string to_string() const;
    };

} // namespace ssp4sim::analysis