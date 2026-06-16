#pragma once

#include "analysis_component.hpp"

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

    class AnalysisModel : public AnalysisComponent
    {
    public:
        std::string type;
        std::string source_file;

        std::vector<std::unique_ptr<AnalysisConnector>> connectors;
        std::vector<std::unique_ptr<AnalysisModelVariable>> model_variables;

        uint64_t delay = 0;
        handler::FmuInfo *fmu = nullptr;

        bool canInterpolateInputs = false;
        int maxOutputDerivativeOrder = 0;


        AnalysisModel() = default;

        AnalysisModel(handler::FmuInfo *fmu_);

        AnalysisModel(handler::FmuInfo *fmu_, const std::string &model_name);

        AnalysisModel(std::string name_, std::string source_file_, handler::FmuInfo *fmu_);

        ~AnalysisModel();

        AnalysisModel(AnalysisModel &&) = default;
        AnalysisModel &operator=(AnalysisModel &&) = default;

        AnalysisModel(const AnalysisModel &) = delete;
        AnalysisModel &operator=(const AnalysisModel &) = delete;

        std::string to_string() const;

    private:
        void create_connectors();
        void create_model_variables();

    };

} // namespace ssp4sim::analysis