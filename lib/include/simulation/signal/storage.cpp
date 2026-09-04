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

    /**
     * @brief Constructor: fix the ring capacity and give the storage a name.
     *
     * Records \p areas (number of time-versioned areas) and \p name, and sets
     * up logging. No memory is allocated yet; call add() then allocate().
     */
    SignalStorage::SignalStorage(std::size_t areas, std::string name)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.signal.SignalStorage"))
    {
        this->areas = areas;
        this->name = std::move(name);
    }

    /** @brief Teardown: destroy any in-place strings, then release memory. */
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

    /**
     * @brief Declare a new variable to store in every area.
     *
     * Computes the value/derivative byte layout (positions, sizes, alignments)
     * within one area, updates the running area_byte_size, and appends a SignalInfo.
     *
     * @param name                     variable name
     * @param type                     FMI data type of the value
     * @param max_interpolation_order  highest derivative order to reserve (0 = none)
     * @return the new variable's index (position in `variables`)
     *
     * Must be called before allocate() for every stored variable.
     */
    size_t SignalStorage::add_variable(std::string name, types::DataType type, size_t max_interpolation_order)
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

        // area_byte_size tracks the growing buffer mapping
        d.position = utils::align_up(this->area_byte_size, d.type_alignment);

        // always align up to derivative type even if there is no derivatives to avoid test branching
        d.derivative_position = utils::align_up(d.position + d.type_size, d.derivative_alignment);

        const auto end_position = d.derivative_position + max_interpolation_order * derivative_size;

        d.total_size = end_position - d.position;

        constexpr std::size_t area_alignment = alignof(std::max_align_t);
        this->area_byte_size = utils::align_up(end_position, area_alignment); // align up to the longest target alignment

        variables.push_back(std::move(d));

        return variables.size() - 1; // the index is the position -1
    }

    /**
     * @brief Allocate the backing ring and precompute all value/derivative pointers.
     *
     * Creates the RingBuffer(areas, area_byte_size), resizes locations and
     * derivative_locations to [areas][#variables], fills the per-area pointers
     * from the offsets computed in add_variable(), and constructs any string variables
     * in place. Marks the storage allocated.
     *
     * @throws std::runtime_error if called more than once.
     */
    void SignalStorage::allocate()
    {
        if (allocated)
        {
            LOG_ERROR(log, "[{func}] Buffer can only be allocated once", __func__);
            throw std::runtime_error("Buffer can only be allocated once");
        }

        ring = std::make_unique<utils::RingBuffer>(this->areas, this->area_byte_size);

        locations.clear();
        derivative_locations.clear();

        locations.resize(areas);
        derivative_locations.resize(areas);

        for (std::size_t area_index = 0; area_index < areas; area_index++)
        {
            std::byte *area = ring->get_item(area_index, false);

            for (auto variable : this->variables)
            {
                locations[area_index].push_back(&area[variable.position]);
                derivative_locations[area_index].push_back(&area[variable.derivative_position]);

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

    /**
     * @brief Advance to a new area stamped with \p time and return its index.
     * Thin wrapper over ring->push(time).
     */
    size_t SignalStorage::push(uint64_t time)
    {
        return ring->push(time);
    }

    /**
     * @brief Return the area for \p time, creating it if it does not exist.
     *
     * Reuses an exact-time match when present (avoids duplicate areas for one
     * timestamp); otherwise pushes a fresh area and returns its index.
     */
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

    /**
     * @brief Exact-time lookup. Delegates to ring->find_exact_index.
     * @return true (with \p found_index set) if an area exactly matches \p time.
     */
    bool SignalStorage::find_area(uint64_t time, size_t &found_index)
    {
        return ring->find_exact_index(time, found_index);
    }

    /**
     * @brief Newest area with timestamp <= \p time. Delegates to ring->find_latest_valid_index.
     * @return true (with \p found_index set) if such an area exists.
     */
    bool SignalStorage::find_latest_valid_area(uint64_t time, size_t &found_index)
    {
        return ring->find_latest_valid_index(time, found_index);
    }

    /**
     * @brief Newest area with timestamp > \p time. Delegates to ring->find_next_valid_index.
     * @return true (with \p found_index set) if such an area exists.
     */
    bool SignalStorage::find_next_valid_area(uint64_t time, size_t &found_index)
    {
        return ring->find_next_valid_index(time, found_index);
    }

    /** @brief Timestamp recorded for \p area. May throw if never populated. */
    std::uint64_t SignalStorage::get_time(std::size_t area)
    {
        return ring->get_time(area);
    }

    /**
     * @brief Cached value pointer for (area, variable).
     * @return byte* to the variable's value, or nullptr if never allocated.
     * Hot path: single lookup, no offset math.
     */
    std::byte *SignalStorage::get_item(std::size_t area, std::size_t index) noexcept
    {
        if (allocated) [[likely]]
        {
            return locations[area][index];
        }
        return nullptr;
    }

    /**
     * @brief Pointer to the `order`-th derivative of variable `index` in `area`.
     * @return byte*, or nullptr on any invalid input (order == 0, order beyond
     *         max_interpolation_orders, bad index, unallocated storage).
     *
     * Bounds/order checks are compiled only under SSP4SIM_HOT_PATH_CHECKS;
     * otherwise computes derivative_locations[area][index] + (order-1)*sizeof(double)
     * directly.
     */
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

        if (derivative_locations[area][index] == nullptr) [[unlikely]]
        {
            return nullptr;
        }
#endif

        return derivative_locations[area][index] + (order - 1) * derivative_size;
    }

    /**
     * @brief Register the single new-data callback with its context.
     * A later call replaces the previous registration.
     */
    void SignalStorage::register_callback(Callback cb, void *context)
    {
        new_data_callback = cb;
        new_data_callback_context = context;
    }

    /**
     * @brief Announce that \p area has fresh data.
     *
     * If allocated and a callback is registered, builds a NewDataEvent
     * (storage, area, timestamp) and invokes the callback with its context.
     * The recorder hooks this to pick up newly written data for export.
     */
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

    /**
     * @brief Human-readable summary of the storage and its variables.
     */
    std::string SignalStorage::to_string() const
    {
        std::ostringstream oss;
        oss << "SignalStorage \n{\n"
            << " name: " << name
            << "  areas: " << areas
            << ", allocated: " << allocated
            << ", total memory size: " << area_byte_size
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

    /**
     * @brief Serialize one area's variables and their current values.
     * For debugging/export: dumps position, derivative position/orders, name,
     * type, size, and the formatted value via the FMI2 type-to-string helper.
     */
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
                << ", der_position " << var.derivative_position
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
