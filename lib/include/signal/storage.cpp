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

    SignalStorage::SignalStorage(std::size_t areas, std::string name)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.signal.SignalStorage"))
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
        d.type_size = ssp4sim::ext::fmi2::enums::get_data_type_size(type);
        d.type_alignment = utils::get_value_alignment(type);
        
        d.max_interpolation_orders = max_interpolation_order;
        d.derivative_size = ssp4sim::ext::fmi2::enums::get_data_type_size(types::DataType::real);
        d.derivative_alignment = utils::get_value_alignment(types::DataType::real);

        // memsize tracks the growing buffer mapping
        d.position = utils::align_up(this->mem_size, d.type_alignment);

        // always align up to derivative type even if there is no derivatives to avoid test branching
        d.derivate_position = utils::align_up(d.position + d.type_size, d.derivative_alignment);

        const auto end_position = d.derivate_position + max_interpolation_order * derivative_size;

        d.total_size = end_position - d.position;

        constexpr std::size_t area_alignment = alignof(std::max_align_t);
        this->mem_size = utils::align_up(end_position, area_alignment); // align up to the longest target alignment

        variables.push_back(std::move(d));

        return variables.size() - 1; // the index is the position -1
    }

    void SignalStorage::allocate()
    {
        if (allocated)
        {
            LOG_ERROR(log, "[{func}] Buffer can only be allocated once", __func__);
            throw std::runtime_error("Buffer can only be allocated once");
        }

        data = std::make_unique<utils::RingBuffer>(this->areas, this->mem_size);

        locations.clear();
        derivate_locations.clear();

        locations.resize(areas);
        derivate_locations.resize(areas);

        for (std::size_t area_index = 0; area_index < areas; area_index++)
        {
            std::byte *area = data->get_item(area_index, false);

            for (auto variable : this->variables)
            {
                locations[area_index].push_back(&area[variable.position]);
                derivate_locations[area_index].push_back(&area[variable.derivate_position]);

                if (variable.type == types::DataType::string)
                {
                    LOG_DEBUG(log, "[{func}] Setting string {index}:{name} - {type}", __func__, variable.index, variable.name, variable.type.to_string());
                    auto s = reinterpret_cast<std::string *>(locations[area_index][variable.index]);
                    std::construct_at(s);
                }
            }
        }
        LOG_DEBUG(log, "[{func}] {storage}", __func__, this->to_string());

        allocated = true;
    }

    size_t SignalStorage::push(uint64_t time)
    {
        return data->push(time);
    }

    size_t SignalStorage::reserve()
    {
        return data->reserve();
    }

    void SignalStorage::commit(size_t area, uint64_t time)
    {
        data->commit(area, time);
    }

    size_t SignalStorage::get_or_push(uint64_t time)
    {
        size_t area;
        if (find_area(time, area))
        {
            return area;
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
        if (allocated) [[likely]]
        {
            return locations[area][index];
        }
        return nullptr;
    }

    std::byte *SignalStorage::get_derivative(std::size_t area, std::size_t index, std::size_t order) noexcept
    {
#ifdef SSP4SIM_HOT_PATH_CHECKS
        if (!allocated)  [[unlikely]]
        {
            return nullptr;
        }

        if (order == 0)  [[unlikely]]
        {
            return nullptr;
        }

        if (index >= variables.size())  [[unlikely]]
        {
            return nullptr;
        }

        const auto max_order = variables[index].max_interpolation_orders;
        if (max_order == 0 || order > max_order)  [[unlikely]]
        {
            return nullptr;
        }

        if (derivate_locations[area][index] == nullptr) [[unlikely]]
        {
            return nullptr;
        }
#endif

        return derivate_locations[area][index] + (order - 1) * derivative_size;
    }

    void SignalStorage::register_callback(Callback cb, void *context)
    {
        new_data_callback = cb;
        new_data_callback_context = context;
    }

    void SignalStorage::flag_new_data(std::size_t area)
    {
        if (allocated) [[likely]]
        {
            // Prep for non flag solution
            if (new_data_callback) [[likely]]
            {
                auto t = NewDataEvent();
                t.storage = this;
                t.area = area;
                t.timestamp = get_time(area);
                new_data_callback(new_data_callback_context, t);
            }
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
