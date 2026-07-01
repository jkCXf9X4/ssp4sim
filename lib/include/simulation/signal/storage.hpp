#pragma once

#include "utils/primitives/ring_buffer.hpp"

#include "pre/1_ssp_parser/schema_extensions/FMI2_Enums_Ext.hpp"

#include "ssp4sim_definitions.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <atomic>

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
        std::string name;
        types::DataType type;
        size_t type_size;
        size_t type_alignment; 

        size_t max_interpolation_orders;
        size_t derivative_size;
        size_t derivative_alignment;

        size_t total_size; // size of data and derivate

        size_t position;          // position in the item data chunk
        size_t derivate_position; // position of the first derivate in the item data chunk
    };

    class SignalStorage; // Forward

    struct NewDataEvent
    {
        // populated by SignalStorage
        SignalStorage *storage = nullptr;
        std::size_t area = 0;
        std::uint64_t timestamp = 0;
        // populated by DataRecorder
        std::byte *buffer = nullptr;
        std::size_t recorder_storage_index = 0;
    };
    using Callback = void (*)(void *, NewDataEvent);


    class SignalStorage : public types::IWritable
    {
    public:
        ssp4cpp::utils::log::Logger *log = nullptr;
        
        std::size_t areas = 0;
        std::string name;

        bool allocated = false;

        std::unique_ptr<utils::RingBuffer> data;

        std::vector<SignalInfo> variables;
        size_t mem_size = 0;

        std::vector<std::vector<std::byte *>> locations;          // absolute location in memory
        std::vector<std::vector<std::byte *>> derivate_locations; // absolute location in memory

        Callback new_data_callback = nullptr;
        void *new_data_callback_context = nullptr;

        SignalStorage(std::size_t areas, std::string name);

        ~SignalStorage();

        size_t add(std::string name, types::DataType type, size_t max_interpolation_order);

        void allocate();

        size_t push(uint64_t time);

        size_t get_or_push(uint64_t time);

        bool find_area(uint64_t time, size_t &found_index);

        bool find_latest_valid_area(uint64_t time, size_t &found_index);

        std::uint64_t get_time(std::size_t area);

        std::byte *get_item(std::size_t area, std::size_t index) noexcept;

        std::byte *get_derivative(std::size_t area, std::size_t index, std::size_t order) noexcept;

        void register_callback(Callback cb, void *context);

        void flag_new_data(std::size_t area);

        std::string to_string() const override;

        std::string export_area(int area);
    };
}
