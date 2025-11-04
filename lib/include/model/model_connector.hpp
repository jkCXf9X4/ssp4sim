#pragma once

#include "utils/node.hpp"
#include "utils/map.hpp"
#include "utils/time.hpp"

#include "ssp4sim_definitions.hpp"

#include "FMI2_Enums_Ext.hpp"

#include "data_ring_storage.hpp"
#include "data_recorder.hpp"
#include "invocable.hpp"
#include "config.hpp"

#include "fmu_handler.hpp"

#include "ssp4sim_definitions.hpp"

#include <string>
#include <vector>
#include <functional>

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
        utils::RingStorage *storage;

        std::unique_ptr<std::byte[]> initial_value;

        bool forward_derivatives = false;
        int forward_derivatives_order = 0;

        void print(std::ostream &os) const override;

        static void set_start_values(std::map<std::string, ConnectorInfo> &connectors);

        static void set_initial_input_area(ssp4sim::utils::RingStorage *input_area,
                                           std::map<std::string, ConnectorInfo> &inputs,
                                           uint64_t time);

        static void write_data_to_model(std::map<std::string, ConnectorInfo> &inputs,
                                        ssp4sim::utils::RingStorage *storage,
                                        int area);

        static void read_values_from_model(std::map<std::string, ConnectorInfo> &outputs,
                                           ssp4sim::utils::RingStorage *storage,
                                           int area);

        static void apply_input_derivatives(std::map<std::string, ConnectorInfo> &inputs,
                                            std::size_t area);

        static void fetch_output_derivatives(std::map<std::string, ConnectorInfo> &outputs,
                                             std::size_t area);
    };
}
