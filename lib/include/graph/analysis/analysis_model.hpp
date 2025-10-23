#pragma once

#include "utils/node.hpp"
#include "utils/map.hpp"
#include "ssp4cpp/utils/string.hpp"
#include "utils/time.hpp"

#include "analysis_connector.hpp"

#include <string>
#include <vector>
#include <map>

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

        AnalysisModel() {}

        AnalysisModel(std::string name, std::string fmu_name, handler::FmuInfo *fmu)
        {
            this->fmu = fmu;
            this->name = name;
            this->fmu_name = fmu_name;
        }

        ~AnalysisModel()
        {
            log(ext_trace)("[{}] Destroying AnalysisModel", __func__);
        }

        void set_interpolation_data(bool canInterpolateInputs, int maxOutputDerivativeOrder)
        {
            this->canInterpolateInputs = canInterpolateInputs;
            this->maxOutputDerivativeOrder = maxOutputDerivativeOrder;
        }

        virtual void print(std::ostream &os) const
        {
            os << "Model { \n"
               << "\nName: " << name
               << "\nFmu: " << fmu_name
               << "\n}\n";
        }
    };
}