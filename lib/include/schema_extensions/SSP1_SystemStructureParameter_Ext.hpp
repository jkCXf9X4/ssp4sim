

#pragma once

#include "cutecpp/log.hpp"

#include "ssp4cpp/schema/fmi2/FMI2_modelDescription.hpp"
#include "ssp4cpp/schema/ssp1/SSP1_SystemStructureDescription.hpp"

#include "ssp4cpp/utils/interface.hpp"

#include "ssp4cpp/schema/ssp1/SSP1_SystemStructureParameterValues.hpp"
#include "ssp4cpp/schema/ssp1/SSP1_SystemStructureParameterMapping.hpp"

#include "FMI2_Enums_Ext.hpp"
#include "ssp4cpp/ssp.hpp"

#include <memory>
#include <cstring>
#include <filesystem>

namespace ssp4sim::ext::ssp1::ssv
{
    inline auto log = Logger("ssp4sim.ext.ssp.ssp1.ssv", LogLevel::info);

    using namespace ssp4cpp::ssp1::ssv;
    using namespace ssp4cpp::ssp1::ssm;

    using DataType = ssp4cpp::fmi2::md::Type;

    struct StartValue : public ssp4cpp::utils::interfaces::IString
    {
        std::string name;
        std::vector<std::string> mappings; // name + mappings
        DataType type;
        size_t size;
        std::unique_ptr<std::byte[]> value;

        StartValue(std::string name, DataType type)
        {
            this->name = name;
            this->type = type;
            this->size = fmi2::enums::get_data_type_size(type);
            this->value = std::make_unique<std::byte[]>(this->size);

            mappings.push_back(name);
        }

        // copystructor
        StartValue(const StartValue &other)
        {
            name = other.name;
            type = other.type;
            size = other.size;
            if (other.value)
            {
                value = std::make_unique<std::byte[]>(size);
                std::memcpy(value.get(), other.value.get(), size);
            }
        }

        // Copy assignment operator
        StartValue &operator=(const StartValue &other)
        {
            if (this == &other) // self-assignment check
                return *this;

            name = other.name;
            mappings = other.mappings;
            type = other.type;
            size = other.size;

            if (other.value)
            {
                value = std::make_unique<std::byte[]>(size);
                std::copy(other.value.get(), other.value.get() + size, value.get());
            }
            else
            {
                value.reset();
            }

            return *this;
        }

        StartValue(StartValue &&other) noexcept = default;

        std::unique_ptr<std::byte[]> get_value()
        {
            auto v = std::make_unique<std::byte[]>(size);
            std::memcpy(v.get(), value.get(), size);
            return std::move(v);
        }

        void store_value(void *value)
        {
            if (this->type == DataType::string)
            {
                log(ext_trace)("[{}] Storing value {}", __func__, *(std::string *)value);
                
                auto s = (std::string *)this->value.get();
                *s = *(std::string *)value;
            }
            else
            {
                memcpy((void *)this->value.get(), value, this->size);
            }
        }

        virtual void print(std::ostream &os) const
        {
            os << "Model { \n"
               << "\nName: " << name
               << "\ntype: " << type.to_string()
               << "\nValue: " << ext::fmi2::enums::data_type_to_string(type, value.get())
               << "\n}\n";
        }

    };


    inline DataType get_parameter_type(TParameter &par)
    {
        if (par.Boolean.has_value())
        {
            return DataType::boolean;
        }
        else if (par.Enumeration.has_value())
        {
            return DataType::enumeration;
        }
        else if (par.Integer.has_value())
        {
            return DataType::integer;
        }
        else if (par.Real.has_value())
        {
            return DataType::real;
        }
        else if (par.String.has_value())
        {
            return DataType::string;
        }
        else
        {
            throw std::runtime_error("Unknown type");
        }
    }

    inline void *get_parameter_value(TParameter &par)
    {
        if (par.Boolean.has_value())
        {
            return &par.Boolean.value().value;
        }
        else if (par.Enumeration.has_value())
        {
            return &par.Enumeration.value().value;
        }
        else if (par.Integer.has_value())
        {
            return &par.Integer.value().value;
        }
        else if (par.Real.has_value())
        {
            return &par.Real.value().value;
        }
        else if (par.String.has_value())
        {
            return &par.String.value().value;
        }
        return nullptr;
    }

    std::map<std::string, StartValue> get_start_value_mappings(ssp4cpp::Ssp &ssp);

}
