#pragma once

#include "analysis_parameter_bindings.hpp"

#include "analysis_component.hpp"

#include "analysis/components/analysis_connector.hpp"
#include "analysis/components/analysis_model_variable.hpp"

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

        std::vector<std::unique_ptr<AnalysisConnector>> connectors;
        std::vector<std::unique_ptr<AnalysisModelVariable>> model_variables;

        std::unique_ptr<AnalysisParameterBindings> parameter_bindings;

        uint64_t delay = 0;
        std::unique_ptr<handler::FmuInfo> fmu;

        bool canInterpolateInputs = false;
        int maxOutputDerivativeOrder = 0;

        AnalysisModel() = default;

        AnalysisModel(std::string name_, std::string source_path, std::unique_ptr<AnalysisParameterBindings> parameter_bindings_);

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