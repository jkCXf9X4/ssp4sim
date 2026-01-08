#include "graph/analysis/analysis_model.hpp"

#include "handler/fmu_handler.hpp"

#include <sstream>

namespace ssp4sim::analysis::graph
{

    AnalysisModel::AnalysisModel() = default;

    AnalysisModel::AnalysisModel(std::string name, std::string fmu_name, handler::FmuInfo *fmu)
    {
        this->fmu = fmu;
        this->name = name;
        this->fmu_name = fmu_name;
    }

    AnalysisModel::~AnalysisModel()
    {
        log(ext_trace)("[{}] Destroying AnalysisModel", __func__);
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
