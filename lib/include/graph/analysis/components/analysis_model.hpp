// DEPRECATED: Use lib/include/analysis/analysis_model.hpp instead.
// This file is kept for backward compatibility. New code should include
// "analysis/analysis_model.hpp" from namespace ssp4sim::analysis.
#pragma once

#include "utils/node.hpp"

#include "handler/fmu_handler.hpp"

#include "analysis_connector.hpp"

#include "ssp4cpp/ssp.hpp"

#include <cstdint>
#include <string>
#include <vector>
#include <map>

namespace ssp4sim::analysis::graph
{

    class AnalysisModel : public ssp4sim::utils::graph::Node
    {
    public:
        uint64_t delay = 0;

        ssp4cpp::utils::log::Logger *log = nullptr;

        std::string fmu_name;
        handler::FmuInfo *fmu;

        std::map<std::string, AnalysisConnector *> connectors;

        bool canInterpolateInputs = false;
        int maxOutputDerivativeOrder = 0;

        AnalysisModel();

        AnalysisModel(std::string name, std::string fmu_name, handler::FmuInfo *fmu);

        ~AnalysisModel();

        void set_interpolation_data(bool canInterpolateInputs, int maxOutputDerivativeOrder);

        std::string to_string() const override;
    };

    std::map<std::string, std::unique_ptr<AnalysisModel>> create_models(ssp4cpp::Ssp &ssp_ref, handler::FmuHandler *fmu_handler, ssp4cpp::utils::log::Logger *log);
}
