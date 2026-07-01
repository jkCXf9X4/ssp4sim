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

    class SspGraphBuilder
    {
    public:
        SspSystemNode system_graph;

        std::vector<std::unique_ptr<SspNode>> node_owner;

        SspGraphBuilder() = default;

        /// Build an SspSystem from an already-loaded SSP and FmuHandler.
        void build(SspSystemNode *tree);
    };

} // namespace ssp4sim::analysis