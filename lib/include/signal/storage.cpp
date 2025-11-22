#include "signal/storage.hpp"

#include "FMI2_Enums_Ext.hpp"

#include <cstring>
#include <sstream>
#include <string>
#include <utility>

namespace ssp4sim::signal
{

    DataStorage::DataStorage(int areas) : new_data_flags(static_cast<std::size_t>(areas))
    {
        this->areas = static_cast<std::size_t>(areas);
    }

    DataStorage::DataStorage(int areas, std::string name) : new_data_flags(static_cast<std::size_t>(areas))
    {
        this->areas = static_cast<std::size_t>(areas);
        this->name = std::move(name);
    }

    uint32_t DataStorage::add(std::string name, types::DataType type, int max_interpolation_order)
    {
        index += 1;
        items += 1;
        auto size = ssp4sim::ext::fmi2::enums::get_data_type_size(type);
        index_name_map[name] = index;

        names.push_back(name);
        types.push_back(type);
        sizes.push_back(size);
        max_interpolation_orders.push_back(static_cast<std::size_t>(max_interpolation_order));

        positions.push_back(pos);
        der_positions.push_back(der_pos);

        pos += size;
        der_pos += static_cast<std::size_t>(max_interpolation_order) * derivative_size;

        return static_cast<uint32_t>(index);
    }

    void DataStorage::allocate()
    {
        locations.clear();
        der_locations.clear();

        total_size = pos * areas;
        data = std::make_unique<std::byte[]>(total_size);
        std::memset(data.get(), 0, total_size);

        auto der_size = der_pos * areas;
        der_data = std::make_unique<std::byte[]>(der_size);
        std::memset(der_data.get(), 0, der_size);

        timestamps.resize(areas);
        locations.resize(areas);
        der_locations.resize(areas);

        for (std::size_t area = 0; area < areas; area++)
        {
            locations[area].reserve(positions.size());
            der_locations[area].reserve(der_positions.size());

            for (auto p_pos : positions)
            {
                locations[area].push_back(&data[area * pos + p_pos]);
            }

            for (std::size_t i = 0; i < der_positions.size(); ++i)
            {
                if (max_interpolation_orders[i] > 0)
                {
                    der_locations[area].push_back(&der_data[area * der_pos + der_positions[i]]);
                }
                else
                {
                    der_locations[area].push_back(nullptr);
                }
            }
            new_data_flags[area] = false;

            for (std::size_t idx = 0; idx < types.size(); idx++)
            {
                if (types[idx] == types::DataType::string)
                {
                    log(debug)("[{}] Setting string {}:{} - {}", __func__, idx, names[idx], types[idx].to_string());
                    auto s = reinterpret_cast<std::string *>(locations[area][idx]);
                    *s = std::string("");
                }
            }
        }

        allocated = true;
    }

    std::byte *DataStorage::get_item(std::size_t area, std::size_t index) noexcept
    {
        if (allocated)
        {
            return locations[area][index];
        }
        return nullptr;
    }

    std::byte *DataStorage::get_derivative(std::size_t area, std::size_t index, std::size_t order) noexcept
    {
        if (allocated && der_locations[area][index] != nullptr)
        {
            return der_locations[area][index] + (order - 1) * derivative_size;
        }
        return nullptr;
    }

    void DataStorage::set_time(std::size_t area, uint64_t time) noexcept
    {
        if (allocated)
        {
            timestamps[area] = time;
        }
    }

    void DataStorage::flag_new_data(std::size_t area)
    {
        if (allocated)
        {
            new_data_flags[area] = true;
        }
    }

    int DataStorage::index_by_name(std::string name)
    {
        return index_name_map[std::move(name)];
    }

    void DataStorage::print(std::ostream &os) const
    {
        os << "DataStorage \n{\n"
           << " name: " << name
           << "  areas: " << areas
           << ", allocated: " << allocated
           << ", total_size: " << total_size
           << ", pos: " << pos
           << ", items: " << items
           << ", index: " << index << "\n";

        for (int i = 0; i < items; i++)
        {
            os << "  { position " << positions[static_cast<std::size_t>(i)]
               << ", name " << names[static_cast<std::size_t>(i)]
               << ", type " << types[static_cast<std::size_t>(i)]
               << ", size " << sizes[static_cast<std::size_t>(i)] << " }\n";
        }
        os << "}";
    }

    std::string DataStorage::export_area(int area)
    {
        std::ostringstream oss;
        oss << "\nArea: \n"
            << area;
        for (int i = 0; i < items; i++)
        {
            auto item = get_item(static_cast<std::size_t>(area), static_cast<std::size_t>(i));
            auto data_type_str = ssp4sim::ext::fmi2::enums::data_type_to_string(types[static_cast<std::size_t>(i)], item);
            oss << "{ position " << positions[static_cast<std::size_t>(i)]
                << ", der_position " << der_positions[static_cast<std::size_t>(i)]
                << ", der_orders " << max_interpolation_orders[static_cast<std::size_t>(i)]
                << ", der_loc " << reinterpret_cast<uint64_t>(der_locations[static_cast<std::size_t>(area)][static_cast<std::size_t>(i)])
                << ", name: " << names[static_cast<std::size_t>(i)]
                << ", type: " << types[static_cast<std::size_t>(i)]
                << ", size: " << sizes[static_cast<std::size_t>(i)]
                << ", value:" << data_type_str
                << " }\n";
        }
        return oss.str();
    }

}

