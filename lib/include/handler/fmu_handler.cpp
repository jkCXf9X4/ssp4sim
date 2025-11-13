#include "handler/fmu_handler.hpp"

#include "SSP_Ext.hpp"
#include "ssp4cpp/ssp.hpp"
#include "ssp4cpp/utils/string.hpp"

#include <memory>
#include <stdexcept>
#include <utility>

namespace ssp4sim::handler
{

    FmuInfo::FmuInfo(std::string name, ssp4cpp::Fmu *fmu)
    {
        this->model_name = fmu->md->modelName;
        this->system_name = name;

        this->fmu = fmu;

        this->fmi_instance = std::make_unique<FmuInstance>(this->fmu->original_file, this->system_name);
        if (!this->fmi_instance->supports_co_simulation())
        {
            throw std::runtime_error(Logger::format("FMU '{}' does not support co-simulation", this->system_name));
        }
        this->model = std::make_unique<CoSimulationModel>(*this->fmi_instance);

        this->model_description = fmu->md.get();
    }

    FmuHandler::FmuHandler(ssp4cpp::Ssp *ssp) : ssp(ssp)
    {
        log(debug)("[{}] Creating FMU map", __func__);
        fmu_map = ssp4sim::ext::ssp::create_fmu_map(*ssp);
        for (auto &[fmu_name, fmu] : fmu_map)
        {
            log(debug)("[{}] - FMU: {} - {}", __func__, fmu_name, fmu->to_string());
        }

        fmu_ref_map = utils::map_ns::map_unique_to_ref(fmu_map);

        log(debug)("[{}] Creating FMU Info map", __func__);
        for (auto &[name, fmu] : fmu_ref_map)
        {
            fmu_info_map.emplace(name, std::make_unique<FmuInfo>(name, fmu));
        }
    }

    void FmuHandler::init()
    {
        log(trace)("[{}] Model init ", __func__);

        log(trace)("[{}] Model init completed", __func__);
    }

}
