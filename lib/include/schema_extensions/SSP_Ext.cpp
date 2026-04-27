
#include "SSP_Ext.hpp"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ssp4sim::ext::ssp
{
    namespace
    {
        ssp4cpp::utils::log::Logger* log()
        {
            // Cache this logger locally so we avoid eager header initialization.
            static ssp4cpp::utils::log::Logger* logger =
                ssp4cpp::utils::log::make_logger("ssp4sim.ext.ssp");
            return logger;
        }
    }

    std::map<std::string, std::string> get_resource_map(ssp4cpp::Ssp &ssp)
    {
        auto resources = std::map<std::string, std::string>();

        for (auto &resource : get_resources(*ssp.ssd))
        {
            auto name = resource->name.value_or("null");
            LOG_TRACE_L1(log(), "Resource {resource} : {source}", name, resource->source);

            resources[name] = resource->source;
        }
        return resources;
    }

    /**
     * @brief Create a map of FMU names to loaded Fmu objects.
     */
    std::map<std::string, std::unique_ptr<ssp4cpp::Fmu>> create_fmu_map(ssp4cpp::Ssp &ssp)
    {
        auto items = std::map<std::string, std::unique_ptr<ssp4cpp::Fmu>>();

        for (auto &resource : get_resources(*ssp.ssd))
        {
            auto name = resource->name.value_or("null");
            LOG_TRACE_L1(log(), "Resource {resource}", name);

            auto fmu = std::make_unique<ssp4cpp::Fmu>(ssp.dir / resource->source);
            items[name] = std::move(fmu);
        }

        LOG_TRACE_L1(log(), "FMUs");
        for (auto &[name, fmu] : items)
        {
            LOG_TRACE_L1(log(), "{} : {}", name, fmu->to_string());
        }
        return items;
    }

    std::vector<ssp4cpp::ssp1::ssd::TComponent *> get_resources(const ssp4cpp::ssp1::ssd::SystemStructureDescription &ssd)
    {
        auto resources = std::vector<ssp4cpp::ssp1::ssd::TComponent *>();

        if (ssd.System.Elements.has_value())
        {
            for (auto &comp : ssd.System.Elements.value().Components)
            {
                // Make sure that the object is cast as a non const
                resources.push_back(const_cast<ssp4cpp::ssp1::ssd::TComponent *>(&comp));
            }
        }
        return resources;
    }

}
