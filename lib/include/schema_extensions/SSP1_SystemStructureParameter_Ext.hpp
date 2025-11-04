

#pragma once

#include "FMI2_Enums_Ext.hpp"

#include "cutecpp/log.hpp"

#include "ssp4cpp/schema/ssp1/SSP1_SystemStructureParameterValues.hpp"


namespace ssp4sim::ext::ssp1::ssv
{
    inline auto log = Logger("ssp4sim.ext.ssp.ssp1.ssv", LogLevel::info);

    // using namespace ssp4cpp::ssp1::ssv;
    // using namespace ssp4cpp::ssp1::ssm;

    struct StartValue : public types::IPrintable
    {
        std::string name;
        std::vector<std::string> mappings; // name + mappings
        types::DataType type;
        size_t size;
        std::unique_ptr<std::byte[]> value;

        virtual void print(std::ostream &os) const
        {
            os << "Model { \n"
               << "\nName: " << name
               << "\ntype: " << type.to_string()
               << "\nValue: " << ext::fmi2::enums::data_type_to_string(type, value.get())
               << "\n}\n";
        }

        StartValue(std::string name, types::DataType type);

        // copystructor
        StartValue(const StartValue &other);

        // Copy assignment operator
        StartValue &operator=(const StartValue &other);

        StartValue(StartValue &&other) noexcept = default;

        std::unique_ptr<std::byte[]> get_value();

        void store_value(void *value);
    };


    types::DataType get_parameter_type(ssp4cpp::ssp1::ssv::TParameter &par);

    void *get_parameter_value(ssp4cpp::ssp1::ssv::TParameter &par);

    std::map<std::string, StartValue> get_start_value_mappings(ssp4cpp::Ssp &ssp);

}
