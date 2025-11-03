#pragma once


#include "utils/node.hpp"
#include "utils/map.hpp"
#include "utils/time.hpp"

#include "FMI2_Enums_Ext.hpp"

#include "data_type.hpp"

#include "ssp4sim_definitions.hpp"

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstddef>
#include <atomic>
#include <cstring>

namespace ssp4sim::utils
{
    /*
     * data centric storage
     * the data storage area should enable:
     * - easy access when exporting results
     * - altering data in and out from the model
     * - store multiple time versions of the data to enable access backwards in time
     */

    class DataStorage : public types::IPrintable
    {
    public:
        Logger log = Logger("ssp4sim.utils.DataStorage", LogLevel::debug);

        // all data
        std::unique_ptr<std::byte[]> data;
        std::unique_ptr<std::byte[]> der_data;

        // these are the same for each timestamp area
        std::vector<std::size_t> positions; // data position relative to start; 0, 4,...
        std::vector<size_t> der_positions;  // data position relative to start; 0, 4,...

        std::vector<types::DataType> types;
        std::vector<std::string> names;
        std::vector<size_t> sizes;
        std::vector<size_t> max_interpolation_orders;

        // for retrieval of index from name
        std::map<std::string, std::size_t> index_name_map;

        std::vector<std::uint64_t> timestamps;
        std::vector<std::vector<std::byte *>> locations;     // absolute location in memory
        std::vector<std::vector<std::byte *>> der_locations; // absolute location in memory
        std::vector<std::atomic<bool>> new_data_flags;

        std::size_t areas = 1;
        std::size_t pos = 0;
        std::size_t der_pos = 0;
        std::size_t total_size = 0;
        std::int32_t index = -1;
        std::size_t items = 0;
        std::string name;
        size_t derivative_size = sizeof(double);

        bool allocated = false;

        explicit DataStorage(int areas);

        DataStorage(int areas, std::string name);

        uint32_t add(std::string name, types::DataType type, int max_interpolation_order);

        void allocate();

        std::byte *get_item(std::size_t area, std::size_t index) noexcept;

        std::byte *get_derivative(std::size_t area, std::size_t index, std::size_t order) noexcept;

        void set_time(std::size_t area, uint64_t time) noexcept;

        void flag_new_data(std::size_t area);

        int index_by_name(std::string name);

        virtual void print(std::ostream &os) const override;

        std::string export_area(int area);
    };
}
