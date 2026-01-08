#pragma once

#include "utils/node.hpp"

#include "handler/fmu_handler.hpp"

#include "analysis_connector.hpp"

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

        Logger log = Logger("ssp4sim.graph.AnalysisModel", LogLevel::info);

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
}
