#pragma once

#include "analysis/components/analysis_connector.hpp"
#include "analysis/components/analysis_model_variable.hpp"

#include "handler/fmu_handler.hpp"

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


        AnalysisModel() = default;

        AnalysisModel(handler::FmuInfo *fmu_, const std::string &model_name = "");

        AnalysisModel(const std::string &name_, const std::string &source_, handler::FmuInfo *fmu_);

        ~AnalysisModel();

        AnalysisModel(AnalysisModel &&) = default;
        AnalysisModel &operator=(AnalysisModel &&) = default;

        AnalysisModel(const AnalysisModel &) = delete;
        AnalysisModel &operator=(const AnalysisModel &) = delete;

        void create_connectors();
        void create_model_variables();

        // void compute_feedthrough(handler::FmuInfo *fmu_info);

        std::string to_string() const;
    };

} // namespace ssp4sim::analysis