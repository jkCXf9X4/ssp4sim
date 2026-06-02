#include "graph/analysis/components/analysis_model.hpp"

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

}
