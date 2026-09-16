
#include "SSP1_SystemStructureDescription_Ext.hpp"

#include "ssp4cpp/utils/string.hpp"
#include "utils/primitives/vector.hpp"
#include "utils/primitives/map.hpp"

#include "ssp4sim_definitions.hpp"

#include <algorithm>
#include <initializer_list>
#include <tuple>
#include <utility>
#include <vector>

using namespace std;
using namespace ssp4cpp::ssp1;
using namespace ssp4cpp::utils::str;
using namespace ssp4cpp::utils;
namespace views = std::ranges::views;

namespace ssp4sim::ext::ssp1
{
    using namespace ssp4cpp::ssp1::ssd;

    namespace
    {
        ssp4cpp::utils::log::Logger *log()
        {
            // Cache this logger locally so we avoid eager header initialization.
            static ssp4cpp::utils::log::Logger *logger =
                ssp4cpp::utils::log::make_logger("ssp4sim.ext.ssp.ssp1.ssv");
            return logger;
        }
    }

    namespace ssd
    {

        // document what is does
        std::map<std::string, TComponent *> get_component_map(const ssp4cpp::ssp1::ssd::SystemStructureDescription &ssd, bool recursive)
        {
            auto resources = std::map<std::string, TComponent *>();

            walk_component(
                ssd.System,
                "",
                [&](const auto &component, const std::string &prefix)
                {
                    auto name_val = prefix + "." + component.name.value_or("unnamed");
                    resources[name_val] = (const_cast<TComponent *>(&component));
                },
                recursive);

            return resources;
        }

        // document what is does
        std::vector<TComponent *> get_components(const ssp4cpp::ssp1::ssd::SystemStructureDescription &ssd, bool recursive)
        {
            auto map = get_component_map(ssd, recursive);
            return utils::map_ns::map_to_value_vector_copy(map);
        }

    }

    namespace elements
    {
        IndexConnectorComponentTuples get_connectors(Elements &elements)
        {
            int i = 0;
            auto cs = IndexConnectorComponentTuples();

            for (auto &component : elements.Components)
            {
                if (component.Connectors.has_value())
                {
                    for (auto &connector : component.Connectors.value().Connectors)
                    {
                        cs.push_back(make_tuple(i, &connector, &component));
                        i++;
                    }
                }
            }

            return cs;
        }

        static void reset_index(IndexConnectorComponentTuples &tuples)
        {
            int i = 0;
            for (auto &[index, connection, component] : tuples)
            {
                index = i;
                i++;
            }
        }

        IndexConnectorComponentTuples get_connectors(
            Elements &elements,
            std::initializer_list<types::Causality> causalities)
        {
            auto in = get_connectors(elements);
            auto out = IndexConnectorComponentTuples();

            std::copy_if(begin(in), end(in), std::back_inserter(out),
                         [causalities](IndexConnectorComponentTuple a)
                         { return utils::list::is_in_list(get<1>(a)->kind, causalities); });

            reset_index(out);

            return out;
        }

        std::set<std::pair<std::string, std::string>> get_fmu_connections(const ssp4cpp::ssp1::ssd::SystemStructureDescription &ssd)
        {
            std::set<std::pair<std::string, std::string>> fmu_connections{};
            if (ssd.System.Connections.has_value())
            {
                for (auto connection : ssd.System.Connections.value().Connections)
                {
                    if (!connection.startElement.has_value() || !connection.endElement.has_value())
                    {
                        LOG_WARNING_LIMIT_EVERY_N(100000, log(), "[{func}] Start or endvalue missing for {connection}", __func__, connection.to_string());
                        continue;
                    }
                    auto p = std::make_pair(connection.startElement.value(), connection.endElement.value());
                    fmu_connections.insert(p);
                }
            }
            return fmu_connections;
        }

    }
}
