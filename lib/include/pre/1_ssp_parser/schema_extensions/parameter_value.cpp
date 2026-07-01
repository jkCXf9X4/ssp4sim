

#include "FMI2_Enums_Ext.hpp"

#include "parameter_value.hpp"


#include <stdexcept>
#include <type_traits>
#include <utility>

namespace ssp4sim::ext
{
    ParameterValue::ParameterValue(std::string name, types::DataType type)
        : name(std::move(name)), type(type)
    {
        // mappings.push_back(this->name);

        size = fmi2::enums::get_data_type_size(type);
        value = fmi2::enums::get_default_value(type);
    }

    std::string ParameterValue::to_string() const
    {
        std::ostringstream oss;
        oss << "Model { \n"
            << "\nName: " << name
            << "\ntype: " << type.to_string()
            << "\nValue: " << ext::fmi2::enums::data_type_to_string(type, const_cast<void *>(raw_ptr()))
            << "\n}\n";
        return oss.str();
    }

    void ParameterValue::store_value(void *raw_value)
    {
        switch (type)
        {
        case types::DataType::real:
            value = *reinterpret_cast<double *>(raw_value);
            break;
        case types::DataType::boolean:
        case types::DataType::integer:
        case types::DataType::enumeration:
            value = *reinterpret_cast<int *>(raw_value);
            break;
        case types::DataType::string:
            value = *reinterpret_cast<std::string *>(raw_value);
            break;
        case types::DataType::unknown:
            value = std::monostate();
            break;
        }
    }

    void *ParameterValue::raw_ptr()
    {
        if (std::holds_alternative<std::monostate>(value))
        {
            return nullptr;
        }

        return std::visit([](auto &v) -> void *
                          { return &v; }, value);
    }

    const void *ParameterValue::raw_ptr() const
    {
        if (std::holds_alternative<std::monostate>(value))
        {
            return nullptr;
        }

        return std::visit([](const auto &v) -> const void *
                          { return &v; }, value);
    }
}
