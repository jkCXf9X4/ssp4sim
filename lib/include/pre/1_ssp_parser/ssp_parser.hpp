#pragma once

#include "ssp_graph_data.hpp"

#include <memory>
#include <string>

namespace ssp4sim::analysis
{

    class SspSystemBuilder
    {
    public:

        SspSystem analysis_system;

        SspSystemBuilder() = default;

        /// Build an SspSystem from an already-loaded SSP and FmuHandler.
        SspSystem build(ssp4cpp::Ssp *ssp);
    };

} // namespace ssp4sim::analysis