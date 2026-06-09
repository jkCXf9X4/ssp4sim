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

        // document what is does
        template <typename SysFn>
        void walk_system(
            const ssp4cpp::ssp1::ssd::TSystem &sys,
            const std::string &path_prefix,
            SysFn &&on_system,
            bool recursive = true)
        {

            on_system(sys, path_prefix);

            if (recursive)
            {
                for (const auto &sub_sys : elements.Systems)
                {
                    const auto sub_sys_name = sub_sys.name.value_or("unnamed");

                    const auto sub_prefix = path_prefix.empty()
                                                ? sub_sys_name
                                                : path_prefix + "." + sub_sys_name;

                    walk_system(sub_sys, sub_prefix, std::forward<SysFn>(on_system), recursive);
                }
            }
        }

        // document what is does
        template <typename ComponentFn>
        void walk_component(
            const ssp4cpp::ssp1::ssd::TSystem &sys,
            const std::string &path_prefix,
            ComponentFn &&on_component,
            bool recursive = true)
        {
            walk_system(
                sys,
                "",
                [&](const auto &system, const std::string &prefix)
                {
                    if (system.Elements.has_value())
                    {
                        const auto sub_sys_name = sub_sys.name.value_or("unnamed");

                        name = prefix.empty()
                                   ? sub_sys_name
                                   : prefix + "." + sub_sys_name;

                        for (auto &comp : system.Elements.value().Components)
                        {
                            on_component(comp, name)
                        }
                    }
                },
                recursive);
        }

        std::vector<TComponent *> get_components(const SystemStructureDescription &ssd, bool recursive = false);

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
