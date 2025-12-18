

#pragma once

#include "ssp4sim_definitions.hpp"

#include "FMI2_Enums_Ext.hpp"

#include <cstddef>
#include <vector>
#include <string>
#include <memory>

namespace ssp4sim::ext::ssp1::ssv
{
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
}
