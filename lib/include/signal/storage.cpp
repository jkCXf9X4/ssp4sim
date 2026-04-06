#include "signal/storage.hpp"


#include <cstring>
#include <memory>
#include <new>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace ssp4sim::signal
{

    const size_t derivative_size = sizeof(double);

    // is the alignment solution to complex?
    // can it just align all to the largest alignas? 
    // 
    namespace
    {
        inline std::size_t align_up(std::size_t value, std::size_t alignment)
        {
            if (alignment <= 1)
            {
                return value;
            }
            const auto remainder = value % alignment;
            if (remainder == 0)
            {
                return value;
            }
            return value + (alignment - remainder);
        }

        inline std::size_t get_value_alignment(types::DataType type)
        {
            switch (type)
            {
            case types::DataType::real:
                return alignof(double);
            case types::DataType::boolean:
            case types::DataType::integer:
            case types::DataType::enumeration:
                return alignof(int);
            case types::DataType::string:
                return alignof(std::string);
            case types::DataType::unknown:
                return alignof(std::byte);
            }
            return alignof(std::byte);
        }
    }

    SignalStorage::SignalStorage(std::size_t areas, std::string name) : new_data_flags(areas)
    {
        this->areas = areas;
        this->name = std::move(name);
    }

    SignalStorage::~SignalStorage()
    {
        if (!allocated)
        {
            return;
        }

        for (std::size_t area_index = 0; area_index < areas; area_index++)
        {
            for (const auto &variable : variables)
            {
                if (variable.type == types::DataType::string)
                {
                    auto value_ptr = reinterpret_cast<std::string *>(locations[area_index][variable.index]);
                    std::destroy_at(value_ptr);
                }
            }
        }
    }


    size_t SignalStorage::add(std::string name, types::DataType type, size_t max_interpolation_order)
    {
        SignalInfo d;
        d.index = variables.size();
        d.name = name;
        d.type = type;
        d.max_interpolation_orders = max_interpolation_order;

        d.type_size = ssp4sim::ext::fmi2::enums::get_data_type_size(type);
        const auto value_alignment = get_value_alignment(type);

        d.position = align_up(this->mem_size, value_alignment);
        if (max_interpolation_order > 0)
        {
            d.derivate_position = align_up(d.position + d.type_size, alignof(double));
        }
        else
        {
            d.derivate_position = d.position + d.type_size;
        }

        const auto end_position = d.derivate_position + max_interpolation_order * derivative_size;
        d.total_size = end_position - d.position;

        constexpr std::size_t area_alignment = alignof(std::max_align_t);
        this->mem_size = align_up(end_position, area_alignment);

        variables.push_back(std::move(d));

        return variables.size() -1;
    }

    void SignalStorage::allocate()
    {
        if (allocated)
        {
            log(error)("[{}] Buffer can only be allocated once", __func__);
            throw std::runtime_error("Buffer can only be allocated once");
        }

        data = std::make_unique<utils::RingBuffer>(this->areas, this->mem_size);

        locations.clear();
        derivate_locations.clear();

        locations.resize(areas);
        derivate_locations.resize(areas);

        for (std::size_t area_index = 0; area_index < areas; area_index++)
        {
            std::byte * area = data->get_item(area_index, false);

            for (auto variable : this->variables)
            {
                locations[area_index].push_back(&area[variable.position]);
                derivate_locations[area_index].push_back(&area[variable.derivate_position]);

                if (variable.type == types::DataType::string)
                {
                    log(debug)("[{}] Setting string {}:{} - {}", __func__, variable.index, variable.name, variable.type.to_string());
                    auto s = reinterpret_cast<std::string *>(locations[area_index][variable.index]);
                    std::construct_at(s);
                }
            }

            new_data_flags[area_index] = false;
        }

        allocated = true;
    }

    size_t SignalStorage::push(uint64_t time)
    {
        return data->push(time);
    }

    size_t SignalStorage::get_or_push(uint64_t time)
    {
        size_t area;
        if (find_area(time, area))
        {
            return static_cast<int>(area);
        }
        auto new_area = push(time);
        return new_area;
    }

    bool SignalStorage::find_area(uint64_t time, size_t &found_index)
    {
        return data->find_index(time, found_index);
    }

    bool SignalStorage::find_latest_valid_area(uint64_t time, size_t &found_index)
    {
        return data->find_latest_valid_index(time, found_index);
    }

    std::uint64_t SignalStorage::get_time(std::size_t area)
    {
        return data->get_time(area);
    }


    std::byte *SignalStorage::get_item(std::size_t area, std::size_t index) noexcept
    {
        if (allocated)
        {
            return locations[area][index];
        }
        return nullptr;
    }

    std::byte *SignalStorage::get_derivative(std::size_t area, std::size_t index, std::size_t order) noexcept
    {
        if (!allocated)
        {
            return nullptr;
        }

        if (order == 0)
        {
            return nullptr;
        }

        if (index >= variables.size())
        {
            return nullptr;
        }

        const auto max_order = variables[index].max_interpolation_orders;
        if (max_order == 0 || order > max_order)
        {
            return nullptr;
        }

        if (derivate_locations[area][index] != nullptr)
        {
            return derivate_locations[area][index] + (order - 1) * derivative_size;
        }

        return nullptr;
    }

    void SignalStorage::flag_new_data(std::size_t area)
    {
        if (allocated)
        {
            new_data_flags[area] = true;
        }
    }


    std::string SignalStorage::to_string() const
    {
        std::ostringstream oss;
        oss << "SignalStorage \n{\n"
            << " name: " << name
            << "  areas: " << areas
            << ", allocated: " << allocated
            << ", total memory size: " << mem_size
            << ", items: " << variables.size();

        for (auto var : variables)
        {
            oss << "  { position " << var.position
                << ", name " << var.name
                << ", type " << var.type.to_string()
                << ", size " << var.type_size << " }\n";
        }
        oss << "}";
        return oss.str();
    }

    std::string SignalStorage::export_area(int area)
    {
        std::ostringstream oss;
        oss << "\nArea: \n"
            << area;
        for (auto var : variables)
        {
            auto item = get_item((area), var.index);
            auto data_str = ssp4sim::ext::fmi2::enums::data_type_to_string(var.type, item);
            oss << "{ position " << var.position
                << ", der_position " << var.derivate_position
                << ", der_orders " << var.max_interpolation_orders
                << ", name: " << var.name
                << ", type: " << var.type.to_string()
                << ", size: " << var.type_size
                << ", value:" << data_str
                << " }\n";
        }
        return oss.str();
    }

}
