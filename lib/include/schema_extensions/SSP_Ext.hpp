#pragma once

#include "ssp4cpp/utils/log.hpp"

#include "ssp4cpp/ssp.hpp"
#include "ssp4cpp/fmu.hpp"

#include <map>
#include <string>
#include <memory>
#include <vector>

namespace ssp4sim::ext::ssp
{
    inline quill::Logger* log = ssp4cpp::utils::log::make_logger("ssp4sim.ext.ssp", quill::LogLevel::TraceL1);

    std::map<std::string, std::string> get_resource_map(ssp4cpp::Ssp &ssp);

    /**
     * @brief Create a map of FMU names to loaded Fmu objects.
     */
    std::map<std::string, std::unique_ptr<ssp4cpp::Fmu>> create_fmu_map(ssp4cpp::Ssp &ssp);

    
    std::vector<ssp4cpp::ssp1::ssd::TComponent *> get_resources(const ssp4cpp::ssp1::ssd::SystemStructureDescription &ssd);

}
