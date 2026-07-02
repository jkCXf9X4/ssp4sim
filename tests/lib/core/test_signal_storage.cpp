#include "simulation/signal/storage.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <stdexcept>

using ssp4sim::signal::SignalStorage;
using ssp4sim::types::DataType;

// ---------------------------------------------------------------------------
// Description: Verifies aligned layout, correct offsets, mem_size stride,
//              and derivative pointer arithmetic
// Rationale:   Misaligned access causes crashes; wrong offsets corrupt data
// ---------------------------------------------------------------------------
TEST_CASE("SignalStorage allocates variable and derivative layout", "[SignalStorage]")
{
    SignalStorage storage(3, "signals");
    const auto real_index = storage.add("signals.real", DataType::real, 2);
    const auto int_index = storage.add("signals.mode", DataType::integer, 0);

    storage.allocate();

    auto area0 = storage.push(100);
    auto *area0_real = storage.get_item(area0, real_index);
    auto *area0_int = storage.get_item(area0, int_index);
    REQUIRE(reinterpret_cast<std::uintptr_t>(area0_real) % alignof(double) == 0);
    REQUIRE(reinterpret_cast<std::uintptr_t>(area0_int) % alignof(int) == 0);
    REQUIRE(reinterpret_cast<std::byte *>(area0_int) - reinterpret_cast<std::byte *>(area0_real) ==
            static_cast<std::ptrdiff_t>(storage.variables[int_index].position - storage.variables[real_index].position));

    auto area1 = storage.push(200);
    auto *area1_real = storage.get_item(area1, real_index);
    REQUIRE(reinterpret_cast<std::uintptr_t>(area1_real) % alignof(double) == 0);
    REQUIRE(reinterpret_cast<std::byte *>(area1_real) - reinterpret_cast<std::byte *>(area0_real) == static_cast<std::ptrdiff_t>(storage.mem_size));

    auto *first_derivative = storage.get_derivative(area0, real_index, 1);
    auto *second_derivative = storage.get_derivative(area0, real_index, 2);
    REQUIRE(first_derivative != nullptr);
    REQUIRE(second_derivative != nullptr);
    REQUIRE(reinterpret_cast<std::uintptr_t>(first_derivative) % alignof(double) == 0);
    REQUIRE(reinterpret_cast<std::uintptr_t>(second_derivative) % alignof(double) == 0);
    REQUIRE(second_derivative - first_derivative == static_cast<std::ptrdiff_t>(sizeof(double)));
}

// ---------------------------------------------------------------------------
// Description: Verifies alignment for mixed types (int, double, string)
// Rationale:   Mixed-type alignment is the most error-prone layout scenario
// ---------------------------------------------------------------------------
TEST_CASE("SignalStorage aligns mixed-type values and derivatives", "[SignalStorage]")
{
    SignalStorage storage(2, "signals");
    const auto int_index = storage.add("signals.mode", DataType::integer, 0);
    const auto real_index = storage.add("signals.real", DataType::real, 1);
    const auto string_index = storage.add("signals.label", DataType::string, 0);
    storage.allocate();

    REQUIRE(storage.variables[int_index].position % alignof(int) == 0);
    REQUIRE(storage.variables[real_index].position % alignof(double) == 0);
    REQUIRE(storage.variables[real_index].derivate_position % alignof(double) == 0);
    REQUIRE(storage.variables[string_index].position % alignof(std::string) == 0);
    REQUIRE(storage.mem_size % alignof(std::max_align_t) == 0);

    const auto area0 = storage.push(10);
    const auto area1 = storage.push(20);

    auto *area0_real = storage.get_item(area0, real_index);
    auto *area1_real = storage.get_item(area1, real_index);
    auto *area0_string = storage.get_item(area0, string_index);
    auto *area1_string = storage.get_item(area1, string_index);

    REQUIRE(reinterpret_cast<std::uintptr_t>(area0_real) % alignof(double) == 0);
    REQUIRE(reinterpret_cast<std::uintptr_t>(area1_real) % alignof(double) == 0);
    REQUIRE(reinterpret_cast<std::uintptr_t>(area0_string) % alignof(std::string) == 0);
    REQUIRE(reinterpret_cast<std::uintptr_t>(area1_string) % alignof(std::string) == 0);
}

// ---------------------------------------------------------------------------
// Description: Verifies push/get_time/find_area/find_latest_valid_area
//              through wraparound
// Rationale:   Timestamp-based lookup is the primary access pattern
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Description: Verifies null returns for out-of-range derivative requests
// Rationale:   Safety — prevents null-pointer dereference
// ---------------------------------------------------------------------------
TEST_CASE("SignalStorage returns null for derivative requests outside valid range", "[SignalStorage]")
{
    SignalStorage storage(2, "signals");
    const auto no_derivative_index = storage.add("signals.mode", DataType::integer, 0);
    const auto derivative_index = storage.add("signals.real", DataType::real, 2);

    REQUIRE(storage.get_item(0, 0) == nullptr);
    REQUIRE(storage.get_derivative(0, derivative_index, 1) == nullptr);

    storage.allocate();
    const auto area = storage.push(100);

    REQUIRE(storage.get_derivative(area, no_derivative_index, 1) == nullptr);
    REQUIRE(storage.get_derivative(area, derivative_index, 0) == nullptr);
    REQUIRE(storage.get_derivative(area, derivative_index, 1) != nullptr);
    REQUIRE(storage.get_derivative(area, derivative_index, 2) != nullptr);
    REQUIRE(storage.get_derivative(area, derivative_index, 3) == nullptr);
}

// ---------------------------------------------------------------------------
// Description: Verifies double-allocate throws std::runtime_error
// Rationale:   Double allocation corrupts memory layout
// ---------------------------------------------------------------------------
TEST_CASE("SignalStorage allocate can only be called once", "[SignalStorage]")
{
    SignalStorage storage(2, "signals");
    storage.add("signals.value", DataType::integer, 0);

    REQUIRE_NOTHROW(storage.allocate());
    REQUIRE_THROWS_AS(storage.allocate(), std::runtime_error);
}

// ---------------------------------------------------------------------------
// Description: Verifies get_or_push reuses areas for the same timestamp
// Rationale:   Coalescing prevents duplicate allocations per timestep
// ---------------------------------------------------------------------------
TEST_CASE("SignalStorage get_or_push reuses existing timestamp areas", "[SignalStorage]")
{
    SignalStorage storage(3, "signals");
    storage.add("signals.temperature", DataType::real, 0);
    storage.allocate();

    const auto first = storage.get_or_push(100);
    const auto second = storage.get_or_push(100);
    const auto third = storage.get_or_push(200);

    REQUIRE(first == second);
    REQUIRE(first != third);

    size_t found = 0;
    REQUIRE(storage.find_area(100, found));
    REQUIRE(found == first);
    REQUIRE(storage.find_area(200, found));
    REQUIRE(found == third);
}
