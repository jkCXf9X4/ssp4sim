#pragma once

#include "ssp4cpp/utils/string.hpp"

#include "utils/node.hpp"

#include "analysis_connector.hpp"

#include <string>
#include <vector>
#include <map>

namespace ssp4sim::handler
{
    struct FmuInfo;
}

namespace ssp4sim::analysis::graph
{

    class AnalysisModel : public ssp4sim::utils::graph::Node
    {
    public:
        uint64_t delay = 0;
        bool is_delay_modeled = false;

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

        void print(std::ostream &os) const override;
    };
}
