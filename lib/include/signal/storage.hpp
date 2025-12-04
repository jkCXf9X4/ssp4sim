#pragma once


// #include "utils/node.hpp"
// #include "utils/map.hpp"
// #include "utils/time.hpp"
#include "utils/ring_buffer.hpp"

#include "FMI2_Enums_Ext.hpp"

// #include "utils/model.hpp"

#include "ssp4sim_definitions.hpp"

#include <string>
#include <vector>
// #include <map>
#include <memory>
// #include <cstddef>
#include <atomic>
// #include <cstring>

namespace ssp4sim::signal
{
    /*
     * data centric storage
     * the data storage area should enable:
     * - easy access when exporting results
     * - altering data in and out from the model
     * - store multiple time versions of the data to enable access backwards in time
     */

     struct SignalInfo
     {
        size_t index;
        types::DataType type;
        std::string name;
        size_t type_size;
        size_t max_interpolation_orders;
        size_t total_size; // size of data and derivate
 
        size_t position; // position in the item data chunk
        size_t derivate_position; // position of the first derivate in the item data chunk

     };

    class SignalStorage : public types::IPrintable
    {
    public:
        Logger log = Logger("ssp4sim.utils.SignalStorage", LogLevel::debug);

        std::unique_ptr<utils::RingBuffer> data;

        std::vector<SignalInfo> variables;
        size_t mem_size = 0;

        std::vector<std::vector<std::byte *>> locations;     // absolute location in memory
        std::vector<std::vector<std::byte *>> derivate_locations; // absolute location in memory
        std::vector<std::atomic<bool>> new_data_flags;

        std::size_t areas = 0;
        std::string name;
        bool allocated = false;

        SignalStorage(std::size_t areas, std::string name);

        size_t add(std::string name, types::DataType type, size_t max_interpolation_order);

        void allocate();

        size_t push(uint64_t time);

        size_t get_or_push(uint64_t time);

        bool find_area(uint64_t time, size_t &found_index);

        bool find_latest_valid_area(uint64_t time, size_t &found_index);

        std::uint64_t get_time(std::size_t area);

        std::byte *get_item(std::size_t area, std::size_t index) noexcept;

        std::byte *get_derivative(std::size_t area, std::size_t index, std::size_t order) noexcept;

        void flag_new_data(std::size_t area);

        virtual void print(std::ostream &os) const override;

        std::string export_area(int area);
    };
}
