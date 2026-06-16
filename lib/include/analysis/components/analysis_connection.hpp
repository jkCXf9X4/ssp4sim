#pragma once

#include "analysis_component.hpp"

#include <cstdint>
#include <string>

namespace ssp4sim::analysis
{

    class AnalysisConnection : public AnalysisComponent
    {
    public:
        std::string source_model;
        std::string source_connector;
        std::string target_model;
        std::string target_connector;

        bool is_boundary_crossing = false;

        uint64_t delay = 0;

        AnalysisConnection() = default;

        AnalysisConnection(std::string source_model_,
                           std::string source_connector_,
                           std::string target_model_,
                           std::string target_connector_,
                           uint64_t delay_ = 0,
                           bool is_boundary_crossing_ = false);

        ~AnalysisConnection() = default;

        void set_custom(uint64_t delay_ = 0)
        {
            delay = delay_;
        }

        std::string to_string() const;

        /// Build a display name for a connection.
        static std::string create_name(const std::string &src_model,
                                       const std::string &src_conn,
                                       const std::string &tgt_model,
                                       const std::string &tgt_conn);
    };

} // namespace ssp4sim::analysis