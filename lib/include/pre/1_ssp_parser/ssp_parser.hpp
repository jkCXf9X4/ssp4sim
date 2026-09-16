#pragma once

#include "../ssp_graph_data.hpp"

#include <memory>
#include <string>

namespace ssp4sim::analysis
{

    class SspSystemBuilder
    {
    public:

        /// Build an SspSystem from an already-loaded SSP.
        /// Eager-loading invariant (IMP-040): this constructor must load every component/connector/connection/nested system up front; graph views built later must never trigger additional SSP object loading.
        SspSystem build(ssp4cpp::Ssp *ssp);
    };

} // namespace ssp4sim::analysis