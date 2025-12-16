#pragma once

#include "ssp4sim_definitions.hpp"

#include "invocable.hpp"

#include "model_connection.hpp"
#include "model_connector.hpp"

#include "signal/storage.hpp"
#include "signal/recorder.hpp"

#include "handler/fmu_handler.hpp"

#include "cutecpp/log.hpp"

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace ssp4sim::graph
{

    class FmuModel final : public Invocable
    {
    public:

        Logger log;

        handler::FmuInfo *fmu;

        std::unique_ptr<ssp4sim::signal::SignalStorage> input_area;
        std::unique_ptr<ssp4sim::signal::SignalStorage> output_area;
        signal::DataRecorder *recorder;

        std::map<std::string, ConnectorInfo> inputs;
        std::map<std::string, ConnectorInfo> outputs;
        std::map<std::string, ConnectorInfo> parameters;
        std::vector<ConnectionInfo> connections;

        bool forward_derivatives = false;
        size_t maxOutputDerivativeOrder = 0;
        bool fmu_logging = false;

        FmuModel(std::string name, ssp4sim::handler::FmuInfo *fmu, size_t maxOutputDerivativeOrder);

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
