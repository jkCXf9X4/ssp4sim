#include "signal/storage.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>

using ssp4sim::signal::SignalStorage;
using ssp4sim::types::DataType;

TEST_CASE("SignalStorage allocates variable and derivative layout", "[SignalStorage]")
{
    SignalStorage storage(3, "signals");
    const auto real_index = storage.add("signals.real", DataType::real, 2);
    const auto int_index = storage.add("signals.mode", DataType::integer, 0);

    storage.allocate();

    const auto expected_real_stride = sizeof(double) + 2 * sizeof(double);
    const auto expected_total_size = expected_real_stride + sizeof(int32_t);
    REQUIRE(storage.mem_size == expected_total_size);

    auto area0 = storage.push(100);
    auto *area0_real = storage.get_item(area0, real_index);
    auto *area0_int = storage.get_item(area0, int_index);
    REQUIRE(reinterpret_cast<std::byte *>(area0_int) - reinterpret_cast<std::byte *>(area0_real) == static_cast<std::ptrdiff_t>(expected_real_stride));

    auto area1 = storage.push(200);
    auto *area1_real = storage.get_item(area1, real_index);
    REQUIRE(reinterpret_cast<std::byte *>(area1_real) - reinterpret_cast<std::byte *>(area0_real) == static_cast<std::ptrdiff_t>(storage.mem_size));

    auto *first_derivative = storage.get_derivative(area0, real_index, 1);
    auto *second_derivative = storage.get_derivative(area0, real_index, 2);
    REQUIRE(first_derivative != nullptr);
    REQUIRE(second_derivative != nullptr);
    REQUIRE(second_derivative - first_derivative == static_cast<std::ptrdiff_t>(sizeof(double)));
}

TEST_CASE("SignalStorage pushes timestamps and finds areas", "[SignalStorage]")
{
    SignalStorage storage(3, "signals");
    storage.add("signals.temperature", DataType::real, 1);
    storage.allocate();

    auto first = storage.push(100);
    auto second = storage.push(200);
    auto third = storage.push(300);

    REQUIRE(storage.get_time(first) == 100);
    REQUIRE(storage.get_time(second) == 200);
    REQUIRE(storage.get_time(third) == 300);

    size_t index_found;
    REQUIRE(storage.find_area(200, index_found) == true);
    REQUIRE(index_found == second);

    REQUIRE(storage.find_latest_valid_area(250, index_found) == true);
    REQUIRE(index_found == second);
    REQUIRE(storage.find_latest_valid_area(50, index_found) == false);

    auto fourth = storage.push(400); // overwrite oldest
    REQUIRE(storage.get_time(fourth) == 400);
    REQUIRE(storage.find_area(100, index_found) == false);

    REQUIRE(storage.find_latest_valid_area(350, index_found) == true);
    REQUIRE(index_found == third);
}

TEST_CASE("SignalStorage flags new data per area", "[SignalStorage]")
{
    SignalStorage storage(2, "signals");
    storage.add("signals.value", DataType::integer, 0);
    storage.allocate();

    for (const auto &flag : storage.new_data_flags)
    {
        REQUIRE(flag == false);
    }

    auto area = storage.push(123);
    storage.flag_new_data(area);

    REQUIRE(storage.new_data_flags[area]);
}
