#pragma once

#include "analysis_component.hpp"
#include "analysis_connector.hpp"

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

        AnalysisConnector *source;
        AnalysisConnector *target;

        uint64_t delay = 0;

        bool is_boundary = false;

        AnalysisConnection() = default;

        AnalysisConnection(std::string source_model_,
                           std::string source_connector_,
                           std::string target_model_,
                           std::string target_connector_);

        // resolved connection
        AnalysisConnection(AnalysisConnector *source_,
                           AnalysisConnector *target_);

        ~AnalysisConnection() = default;

        void set_custom(uint64_t delay_ = 0)
        {
            delay = delay_;
        }

        std::string to_string() const;
    };

} // namespace ssp4sim::analysis