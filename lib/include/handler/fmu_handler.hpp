#pragma once

#include "utils/map.hpp"
#include "utils/vector.hpp"

#include "ssp4cpp/ssp.hpp"
#include "ssp4cpp/fmu.hpp"

#include "fmi4c_adapter.hpp"

#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <list>
#include <format>

namespace ssp4sim::handler
{
    // using namespace std;

    struct FmuInfo
    {
        std::string system_name;
        std::string model_name;

        // Borrowing
        ssp4cpp::Fmu *fmu;
        ssp4cpp::fmi2::md::fmi2ModelDescription *model_description;

        // Owning
        std::unique_ptr<FmuInstance> fmi_instance;
        std::unique_ptr<CoSimulationModel> model;

        FmuInfo(std::string name, ssp4cpp::Fmu *fmu);
        // can not be copied, has unique pointers
        FmuInfo(const FmuInfo &) = delete;
        FmuInfo &operator=(const FmuInfo &) = delete;
    };

    class FmuHandler
    {
    public:
        Logger log = Logger("ssp4sim.handler.FmuHandler", LogLevel::info);

        ssp4cpp::Ssp *ssp;

        std::map<std::string, std::unique_ptr<ssp4cpp::Fmu>> fmu_map;
        std::map<std::string, ssp4cpp::Fmu *> fmu_ref_map; // Non owning

        std::map<std::string, std::unique_ptr<FmuInfo>> fmu_info_map;

        FmuHandler(ssp4cpp::Ssp *ssp);

        void init();
    };
}
