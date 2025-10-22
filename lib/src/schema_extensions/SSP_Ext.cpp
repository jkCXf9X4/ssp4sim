
#include "SSP_Ext.hpp"


namespace ssp4cpp::ssp::ext
{
    std::map<std::string, std::string> get_resource_map(ssp4cpp::Ssp &ssp)
    {
        auto resources = std::map<std::string, std::string>();

        for (auto &resource : ssp4cpp::ssp1::ext::ssd::get_resources(*ssp.ssd))
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
    std::map<std::string, std::unique_ptr<Fmu>> create_fmu_map(ssp4cpp::Ssp &ssp)
    {
        auto items = std::map<std::string, std::unique_ptr<Fmu>>();

        for (auto &resource : ssp4cpp::ssp1::ext::ssd::get_resources(*ssp.ssd))
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

}