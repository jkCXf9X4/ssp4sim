#pragma once

#include "analysis_graph_data.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <memory>
#include <string>

namespace ssp4cpp
{
    class Ssp;
}

namespace ssp4cpp::ssp1::ssd
{
    struct TSystem;
}

namespace ssp4sim::analysis
{

    class AnalysisSystemBuilder
    {
    public:
        ssp4cpp::utils::log::Logger *log = nullptr;

        std::unique_ptr<AnalysisSystem> analysis_system;

        AnalysisGraphData tree;

        AnalysisSystemBuilder() = default;

        /// Build an AnalysisSystem from an already-loaded SSP and FmuHandler.
        std::unique_ptr<AnalysisSystem> build(ssp4cpp::Ssp *ssp);
    };

} // namespace ssp4sim::analysis