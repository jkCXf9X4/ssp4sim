#pragma once

#include "ssp_graph_data.hpp"

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

    class SspTreeBuilder
    {
    public:
        SspSystemNode system_tree;

        std::vector<std::unique_ptr<SspNode>> node_owner;

        SspTreeBuilder() = default;

        /// Build an SspSystem from an already-loaded SSP and FmuHandler.
        void build(SspSystem *analysis_system);
    };

} // namespace ssp4sim::analysis