#include "utils/ring_buffer.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>

using ssp4sim::utils::RingBuffer;

TEST_CASE("RingBuffer rejects zero capacity")
{
    REQUIRE_THROWS_AS(RingBuffer(0, sizeof(std::uint32_t)), std::runtime_error);
}

TEST_CASE("RingBuffer push grows until full then overwrites oldest")
{
    constexpr std::size_t kCapacity = 3;
    RingBuffer buffer(kCapacity, sizeof(std::uint32_t));

    REQUIRE(buffer.is_empty());
    REQUIRE_FALSE(buffer.is_full());

    auto expected_head = (buffer.head + 1) % buffer.capacity;
    auto first_index = buffer.push();
    REQUIRE(first_index == expected_head);
    REQUIRE(buffer.head == expected_head);
    REQUIRE(buffer.nr_items == 1);
    REQUIRE_FALSE(buffer.is_empty());

    expected_head = (buffer.head + 1) % buffer.capacity;
    auto second_index = buffer.push();
    REQUIRE(second_index == expected_head);
    REQUIRE(buffer.nr_items == 2);

    buffer.push();
    REQUIRE(buffer.is_full());
    REQUIRE(buffer.nr_items == buffer.capacity);

    auto previous_tail = buffer.tail;
    auto previous_head = buffer.head;
    buffer.push();
    REQUIRE(buffer.nr_items == buffer.capacity);
    REQUIRE(buffer.tail == (previous_tail + 1) % buffer.capacity);
    REQUIRE(buffer.head == (previous_head + 1) % buffer.capacity);
}

TEST_CASE("RingBuffer get_item provides contiguous storage and bounds checks")
{
    constexpr std::size_t element_size = sizeof(std::uint64_t);
    RingBuffer buffer(2, element_size);

    auto *first = buffer.get_item(0);
    auto *second = buffer.get_item(1);
    REQUIRE(second - first == static_cast<std::ptrdiff_t>(element_size));

    *reinterpret_cast<std::uint64_t *>(first) = 42;
    *reinterpret_cast<std::uint64_t *>(second) = 99;
    REQUIRE(*reinterpret_cast<std::uint64_t *>(buffer.get_item(0)) == 42);
    REQUIRE(*reinterpret_cast<std::uint64_t *>(buffer.get_item(1)) == 99);

    REQUIRE_THROWS_AS(buffer.get_item(buffer.capacity), std::runtime_error);
}
