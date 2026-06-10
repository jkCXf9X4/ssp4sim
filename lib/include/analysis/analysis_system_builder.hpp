#pragma once

#include "analysis/components/analysis_system.hpp"

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

namespace ssp4sim::handler
{
    class FmuHandler;
    struct FmuInfo;
}

namespace ssp4sim::analysis
{

    class AnalysisSystemBuilder
    {
    public:
        ssp4cpp::utils::log::Logger *log = nullptr;

        AnalysisSystemBuilder() = default;

/// Build an AnalysisSystem from an already-loaded SSP and FmuHandler.
        std::unique_ptr<AnalysisSystem> build(ssp4cpp::Ssp *ssp,
                                                handler::FmuHandler *fmu_handler);

        /// Build an AnalysisSystem from an SSP file path (convenience entry point).
        std::unique_ptr<AnalysisSystem> build(const std::string &ssp_path);

        /// Process system-level (boundary) connectors from the SSD system node.
        void process_boundary_connectors(AnalysisSystem &sys,
                                         const ssp4cpp::ssp1::ssd::TSystem &ssd_sys);

        /// Process connections at this system level from the SSD system node.
        void process_connections(AnalysisSystem &sys,
                                 const ssp4cpp::ssp1::ssd::TSystem &ssd_sys);
    };

} // namespace ssp4sim::analysis