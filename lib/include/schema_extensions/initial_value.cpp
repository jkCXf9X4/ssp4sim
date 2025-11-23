#include "initial_value.hpp"

#include <cstring>

namespace ssp4sim::ext::ssp1::ssv
{
    StartValue::StartValue(std::string name, types::DataType type)
    {
        this->name = name;
        this->type = type;
        this->size = fmi2::enums::get_data_type_size(type);
        this->value = std::make_unique<std::byte[]>(this->size);

        mappings.push_back(name);
    }

    // copystructor
    StartValue::StartValue(const StartValue &other)
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
    StartValue &StartValue::operator=(const StartValue &other)
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

    std::unique_ptr<std::byte[]> StartValue::get_value()
    {
        auto v = std::make_unique<std::byte[]>(size);
        std::memcpy(v.get(), value.get(), size);
        return std::move(v);
    }

    void StartValue::store_value(void *value)
    {
        if (this->type == types::DataType::string)
        {
            auto s = (std::string *)this->value.get();
            *s = *(std::string *)value;
        }
        else
        {
            memcpy((void *)this->value.get(), value, this->size);
        }
    }

}
