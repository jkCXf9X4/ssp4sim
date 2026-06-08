#pragma once

#include "analysis/analysis_connector.hpp"
#include "analysis/analysis_model_variable.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ssp4sim::handler
{
    struct FmuInfo;
}

namespace ssp4sim::analysis
{

    class AnalysisModel
    {
    public:
        std::string name;
        std::string type;
        std::string source_file;

        std::vector<std::unique_ptr<AnalysisConnector>> connectors;
        std::vector<std::unique_ptr<AnalysisModelVariable>> model_variables;

        uint64_t delay = 0;
        handler::FmuInfo *fmu = nullptr;

        bool canInterpolateInputs = false;
        int maxOutputDerivativeOrder = 0;

        /// Compute feedthrough marking on this model's connectors from FMU ModelStructure.
        void compute_feedthrough(handler::FmuInfo *fmu_info);

        AnalysisModel() = default;

        AnalysisModel(std::string name_,
                      std::string source_file_,
                      handler::FmuInfo *fmu_);

        ~AnalysisModel();

        AnalysisModel(AnalysisModel &&) = default;
        AnalysisModel &operator=(AnalysisModel &&) = default;

        AnalysisModel(const AnalysisModel &) = delete;
        AnalysisModel &operator=(const AnalysisModel &) = delete;

        std::string to_string() const;
    };

} // namespace ssp4sim::analysis