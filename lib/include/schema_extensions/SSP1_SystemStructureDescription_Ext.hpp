#pragma once

#include "cutecpp/log.hpp"

#include "ssp4sim_definitions.hpp"

#include "ssp4cpp/schema/ssp1/SSP1_SystemStructureDescription.hpp"

#include <string>
#include <vector>
#include <set>

namespace ssp4sim::ext::ssp1
{
    using namespace ssp4cpp::ssp1::ssd;

    inline auto log = Logger("ssp4sim.ext.ssp.ssp1", LogLevel::debug);

    namespace ssd
    {
        inline auto log = Logger("ssp4sim.ext.ssp.ssp1.ssd", LogLevel::debug);

        std::vector<TComponent *> get_resources(const SystemStructureDescription &ssd);
    }

    namespace elements
    {
        inline auto log = Logger("ssp4sim.ext.ssp.ssp1.elements", LogLevel::debug);

        using IndexConnectorComponentTuple = tuple<int, Connector *, TComponent *>;
        using IndexConnectorComponentTuples = vector<IndexConnectorComponentTuple>;

        IndexConnectorComponentTuples get_connectors(Elements &elements);

        IndexConnectorComponentTuples get_connectors(
            Elements &elements,
            std::initializer_list<types::Causality> causalities);

        // Get connections between fmus
        // return a set of <source_fmu, target_fmu> strings
        set<pair<string, string>> get_fmu_connections(const SystemStructureDescription &ssd);
    }

}
