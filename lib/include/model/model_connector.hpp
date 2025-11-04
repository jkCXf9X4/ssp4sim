#pragma once

#include "ssp4sim_definitions.hpp"

#include "cutecpp/log.hpp"
#include "utils/data_type.hpp"

#include <string>
#include <map>
#include <memory>

namespace ssp4sim::handler
{
    struct FmuInfo;
}

namespace ssp4sim::utils
{
    class RingStorage;
}

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

        ssp4sim::handler::FmuInfo *fmu;
        ssp4sim::utils::RingStorage *storage;

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
