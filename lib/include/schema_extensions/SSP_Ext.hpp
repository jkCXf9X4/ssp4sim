#pragma once

#include "cutecpp/log.hpp"

#include "ssp4cpp/ssp.hpp"
#include "ssp4cpp/fmu.hpp"

#include <map>
#include <string>
#include <memory>
#include <vector>

namespace ssp4sim::ext::ssp
{
    inline auto log = Logger("ssp4sim.ext.ssp", LogLevel::info);

    std::map<std::string, std::string> get_resource_map(ssp4cpp::Ssp &ssp);

    /**
     * @brief Create a map of FMU names to loaded Fmu objects.
     */
    std::map<std::string, std::unique_ptr<ssp4cpp::Fmu>> create_fmu_map(ssp4cpp::Ssp &ssp);

    
    std::vector<ssp4cpp::ssp1::ssd::TComponent *> get_resources(const ssp4cpp::ssp1::ssd::SystemStructureDescription &ssd);

}
