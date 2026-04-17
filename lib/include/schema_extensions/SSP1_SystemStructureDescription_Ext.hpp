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

    inline quill::Logger* log = ssp4cpp::utils::log::make_logger("ssp4sim.ext.ssp.ssp1", quill::LogLevel::TraceL1);

    namespace ssd
    {
        inline quill::Logger* log = ssp4cpp::utils::log::make_logger("ssp4sim.ext.ssp.ssp1.ssd", quill::LogLevel::TraceL1);

        std::vector<TComponent *> get_resources(const SystemStructureDescription &ssd);
    }

    namespace elements
    {
        inline quill::Logger* log = ssp4cpp::utils::log::make_logger("ssp4sim.ext.ssp.ssp1.elements", quill::LogLevel::TraceL1);

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
