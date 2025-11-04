
#include "SSP_Ext.hpp"


namespace ssp4sim::ext::ssp
{
    std::map<std::string, std::string> get_resource_map(ssp4cpp::Ssp &ssp)
    {
        auto resources = std::map<std::string, std::string>();

        for (auto &resource : get_resources(*ssp.ssd))
        {
            auto name = resource->name.value_or("null");
            log(trace)("Resource {} : {}", name, resource->source);

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
            log(trace)("Resource {}", name);

            auto fmu = std::make_unique<ssp4cpp::Fmu>(ssp.dir / resource->source);
            items[name] = std::move(fmu);
        }

        log(trace)("FMUs");
        for (auto &[name, fmu] : items)
        {
            log(trace)("{} : {}", name, fmu->to_string());
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