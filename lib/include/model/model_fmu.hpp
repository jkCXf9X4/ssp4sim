#pragma once

#include "ssp4sim_definitions.hpp"

#include "invocable.hpp"

#include "model_connection.hpp"
#include "model_connector.hpp"

#include "cutecpp/log.hpp"

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace ssp4sim::handler
{
    struct FmuInfo;
}

namespace ssp4sim::utils
{
    class RingStorage;
    class DataRecorder;
}

namespace ssp4sim::graph
{

    class FmuModel final : public Invocable
    {
    public:
        bool is_delay_modeled = false;

        Logger log;

        ssp4sim::handler::FmuInfo *fmu;

        std::unique_ptr<ssp4sim::utils::RingStorage> input_area;
        std::unique_ptr<ssp4sim::utils::RingStorage> output_area;
        ssp4sim::utils::DataRecorder *recorder;

        std::map<std::string, ConnectorInfo> inputs;
        std::map<std::string, ConnectorInfo> outputs;
        std::map<std::string, ConnectorInfo> parameters;
        std::vector<ConnectionInfo> connections;

        bool forward_derivatives = false;
        bool fmu_logging = false;

        FmuModel(std::string name, ssp4sim::handler::FmuInfo *fmu);

        ~FmuModel();

        void print(std::ostream &os) const override;

        void enter_init();

        void exit_init();

        uint64_t direct_feedthrough(uint64_t start);

        void pre(uint64_t input_time);

        void post(uint64_t time);

        uint64_t step(StepData step_data);

        uint64_t invoke(StepData step_data) override final;
    };
}
