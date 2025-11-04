#pragma once

#include "utils/node.hpp"
#include "utils/map.hpp"
#include "ssp4cpp/utils/string.hpp"
#include "utils/time.hpp"
#include "utils/timer.hpp"

#include "ssp4sim_definitions.hpp"

#include "FMI2_Enums_Ext.hpp"

#include "data_ring_storage.hpp"
#include "data_recorder.hpp"
#include "invocable.hpp"
#include "config.hpp"

#include "fmu_handler.hpp"
#include "model_connection.hpp"
#include "model_connector.hpp"

#include <string>
#include <vector>
#include <functional>

namespace ssp4sim::graph
{

    class FmuModel final : public Invocable
    {
    public:
        bool is_delay_modeled = false;

        Logger log;

        handler::FmuInfo *fmu;

        std::unique_ptr<utils::RingStorage> input_area;
        std::unique_ptr<utils::RingStorage> output_area;
        utils::DataRecorder *recorder;

        std::map<std::string, ConnectorInfo> inputs;
        std::map<std::string, ConnectorInfo> outputs;
        std::map<std::string, ConnectorInfo> parameters;
        std::vector<ConnectionInfo> connections;

        bool forward_derivatives = utils::Config::getOr<bool>("simulation.executor.forward_derivatives", true);

        FmuModel(std::string name, handler::FmuInfo *fmu);

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
