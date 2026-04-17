#pragma once

#include "ssp4cpp/utils/log.hpp"

#include "ssp4sim_definitions.hpp"

#include "ssp4cpp/schema/ssp1/SSP1_SystemStructureDescription.hpp"

#include <initializer_list>
#include <string>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

namespace ssp4sim::ext::ssp1
{
    using namespace ssp4cpp::ssp1::ssd;

    namespace ssd
    {
        std::vector<TComponent *> get_resources(const SystemStructureDescription &ssd);
    }

    namespace elements
    {
        using IndexConnectorComponentTuple = std::tuple<int, Connector *, TComponent *>;
        using IndexConnectorComponentTuples = std::vector<IndexConnectorComponentTuple>;

        IndexConnectorComponentTuples get_connectors(Elements &elements);

        IndexConnectorComponentTuples get_connectors(
            Elements &elements,
            std::initializer_list<types::Causality> causalities);

        // Get connections between fmus
        // return a set of <source_fmu, target_fmu> strings
        std::set<std::pair<std::string, std::string>> get_fmu_connections(const SystemStructureDescription &ssd);
    }

}
