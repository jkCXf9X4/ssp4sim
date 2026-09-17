#include "fmu_info.hpp"

// #include "SSP_Ext.hpp"

// #include "ssp4cpp/ssp.hpp"

// #include <format>
#include <memory>
#include <stdexcept>

namespace ssp4sim::handler
{
    FmuInfo::FmuInfo(std::string name, std::shared_ptr<ssp4cpp::Fmu> fmu)
    {
        this->model_name = fmu->md->modelName;
        this->system_name = name;

        this->fmu = std::move(fmu);

        this->fmi_instance = std::make_unique<FmuInstance>(this->fmu->dir, this->system_name);
        if (!this->fmi_instance->supports_co_simulation())
        {
            throw std::runtime_error(std::format("FMU '{}' does not support co-simulation", this->system_name));
        }
        this->model = std::make_unique<CoSimulationModel>(*this->fmi_instance);

        this->model_description = this->fmu->md.get();
    }
}
