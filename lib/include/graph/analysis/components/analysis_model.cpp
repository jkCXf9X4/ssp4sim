// DEPRECATED: Use lib/include/analysis/analysis_model.hpp instead.
// This file is kept for backward compatibility.
#include "graph/analysis/components/analysis_model.hpp"

#include "SSP_Ext.hpp"

#include "handler/fmu_handler.hpp"

#include <sstream>

namespace ssp4sim::analysis::graph
{

    AnalysisModel::AnalysisModel()
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.graph.AnalysisModel"))
    {
    }

    AnalysisModel::AnalysisModel(std::string name, std::string fmu_name, handler::FmuInfo *fmu)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.graph.AnalysisModel"))
    {
        this->fmu = fmu;
        this->name = name;
        this->fmu_name = fmu_name;
    }

    AnalysisModel::~AnalysisModel()
    {
        LOG_TRACE_L1(log, "[{func}] Destroying AnalysisModel", __func__);
    }

    void AnalysisModel::set_interpolation_data(bool canInterpolateInputs, int maxDerivativeOrder)
    {
        this->canInterpolateInputs = canInterpolateInputs;
        this->maxOutputDerivativeOrder = maxDerivativeOrder;
    }

    std::string AnalysisModel::to_string() const
    {
        std::ostringstream oss;
        oss << "Model { \n"
            << "\nName: " << name
            << "\nFmu: " << fmu_name
            << "\n}\n";
        return oss.str();
    }

    std::map<std::string, std::unique_ptr<AnalysisModel>> create_models(ssp4cpp::Ssp &ssp_ref, handler::FmuHandler *fmu_handler, ssp4cpp::utils::log::Logger *log)
    {
        LOG_TRACE_L1(log, "[{func}] init", __func__);
        std::map<std::string, std::unique_ptr<AnalysisModel>> models;

        for (auto &resource : ext::ssp::get_resources(*ssp_ref.ssd))
        {
            auto ssp_resource_name = resource->name.value_or("null");

            auto fmu = fmu_handler->fmu_info_map[ssp_resource_name].get();
            auto m = std::make_unique<AnalysisModel>(ssp_resource_name, resource->source, fmu);

            if (fmu->model_description->CoSimulation)
            {
                auto co_sim = *fmu->model_description->CoSimulation;
                m->set_interpolation_data(co_sim.canInterpolateInputs.value_or(false), co_sim.maxOutputDerivativeOrder.value_or(0));
            }

            LOG_DEBUG(log, "[{func}] New Model: {model}", __func__, m->name);
            models[m->name] = std::move(m);
        }
        LOG_TRACE_L1(log, "[{func}] exit", __func__);
        return models;
    }

}
