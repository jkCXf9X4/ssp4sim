
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
        ssp4cpp::utils::log::Logger *log()
        {
            // Cache this logger locally so we avoid eager header initialization.
            static ssp4cpp::utils::log::Logger *logger =
                ssp4cpp::utils::log::make_logger("ssp4sim.ext.ssp");
            return logger;
        }
    }


    namespace
    {
        void create_fmu_map_recursive(
            const ssp4cpp::ssp1::ssd::TSystem &sys,
            const std::string &path_prefix,
            std::map<std::string, std::unique_ptr<ssp4cpp::Fmu>> &items,
            const std::filesystem::path &ssp_dir)
        {

            // switch to the walk_components to code less
            if (!sys.Elements.has_value())
                return;

            for (auto &component : sys.Elements.value().Components)
            {
                if (!component.name.has_value())
                {
                    continue;
                }
                auto component_name = component.name.value();
                auto name = path_prefix.empty() ? component_name : path_prefix + "." + component_name;

                items[name] = std::make_unique<ssp4cpp::Fmu>(ssp_dir / component.source);
            }
            for (auto &sub_sys : sys.Elements.value().Systems)
            {
                auto sub_sys_name = sub_sys.name.value_or("unnamed");
                auto sub_prefix = path_prefix.empty() ? sub_sys_name : path_prefix + "." + sub_sys_name;

                create_fmu_map_recursive(sub_sys, sub_prefix, items, ssp_dir);
            }
        }
    }

    /**
     * @brief Create a map of FMU names to loaded Fmu objects.
     */
    std::map<std::string, std::unique_ptr<ssp4cpp::Fmu>> create_fmu_map(ssp4cpp::Ssp &ssp)
    {
        auto items = std::map<std::string, std::unique_ptr<ssp4cpp::Fmu>>();
        create_fmu_map_recursive(ssp.ssd->System, "", items, ssp.dir);
        LOG_TRACE_L1(log(), "FMUs");
        for (auto &[name, fmu] : items)
        {
            LOG_TRACE_L1(log(), "{} : {}", name, fmu->to_string());
        }
        return items;
    }
}
