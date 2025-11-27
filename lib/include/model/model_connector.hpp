#pragma once

#include "ssp4sim_definitions.hpp"

#include "utils/model.hpp"

#include "signal/storage.hpp"
#include "handler/fmu_handler.hpp"

#include "cutecpp/log.hpp"

#include <string>
#include <map>
#include <memory>

namespace ssp4sim::graph
{

    class ConnectorInfo : public types::IPrintable
    {
    public:
        static Logger log;

        types::DataType type;
        size_t size;
        std::string name;

        uint32_t index;
        uint64_t value_ref;

        handler::FmuInfo *fmu;
        signal::SignalStorage *storage;

        std::unique_ptr<std::byte[]> initial_value;

        bool forward_derivatives = false;
        int forward_derivatives_order = 0;

        void print(std::ostream &os) const override;

        static void set_start_values(std::map<std::string, ConnectorInfo> &connectors);

        static void set_initial_input_area(signal::SignalStorage *input_area,
                                           std::map<std::string, ConnectorInfo> &inputs,
                                           uint64_t time);

        static void write_data_to_model(std::map<std::string, ConnectorInfo> &inputs,
                                        signal::SignalStorage *storage,
                                        int area);

        static void read_values_from_model(std::map<std::string, ConnectorInfo> &outputs,
                                           signal::SignalStorage *storage,
                                           int area);

        static void apply_input_derivatives(std::map<std::string, ConnectorInfo> &inputs,
                                            std::size_t area);

        static void fetch_output_derivatives(std::map<std::string, ConnectorInfo> &outputs,
                                             std::size_t area);
    };
}
