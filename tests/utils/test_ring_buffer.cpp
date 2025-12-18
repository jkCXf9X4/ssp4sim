#include "utils/ring_buffer.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <array>
#include <stdexcept>

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

    const auto previous_tail = buffer.tail;
    const auto previous_head = buffer.head;
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

TEST_CASE("RingBuffer index helpers follow head/tail ordering")
{
    RingBuffer buffer(3, sizeof(int));

    auto idx1 = buffer.push();
    *reinterpret_cast<int *>(buffer.get_item(idx1)) = 10;

    auto idx2 = buffer.push();
    *reinterpret_cast<int *>(buffer.get_item(idx2)) = 20;

    auto idx3 = buffer.push();
    *reinterpret_cast<int *>(buffer.get_item(idx3)) = 30;

    REQUIRE(buffer.is_full());
    REQUIRE(buffer.nr_items == buffer.capacity);

    // Reverse lookup should walk newest to oldest.
    REQUIRE(*reinterpret_cast<int *>(buffer.get_item(buffer.get_index_from_pos_rev(0))) == 30);
    REQUIRE(*reinterpret_cast<int *>(buffer.get_item(buffer.get_index_from_pos_rev(1))) == 20);
    REQUIRE(*reinterpret_cast<int *>(buffer.get_item(buffer.get_index_from_pos_rev(2))) == 10);

    // Forward lookup uses tail-relative ordering.
    std::array<int, 3> forward_values{
        *reinterpret_cast<int *>(buffer.get_item(buffer.get_index_from_pos(0))),
        *reinterpret_cast<int *>(buffer.get_item(buffer.get_index_from_pos(1))),
        *reinterpret_cast<int *>(buffer.get_item(buffer.get_index_from_pos(2)))};
    REQUIRE(forward_values == std::array<int, 3>{30, 10, 20});

    // Overwrite oldest and confirm head/tail advance and value mapping update.
    auto idx4 = buffer.push();
    *reinterpret_cast<int *>(buffer.get_item(idx4)) = 40;

    REQUIRE(buffer.is_full());
    REQUIRE(buffer.nr_items == buffer.capacity);

    REQUIRE(*reinterpret_cast<int *>(buffer.get_item(buffer.get_index_from_pos_rev(0))) == 40);
    REQUIRE(*reinterpret_cast<int *>(buffer.get_item(buffer.get_index_from_pos_rev(1))) == 30);
    REQUIRE(*reinterpret_cast<int *>(buffer.get_item(buffer.get_index_from_pos_rev(2))) == 20);

    std::array<int, 3> forward_after_overwrite{
        *reinterpret_cast<int *>(buffer.get_item(buffer.get_index_from_pos(0))),
        *reinterpret_cast<int *>(buffer.get_item(buffer.get_index_from_pos(1))),
        *reinterpret_cast<int *>(buffer.get_item(buffer.get_index_from_pos(2)))};
    REQUIRE(forward_after_overwrite == std::array<int, 3>{40, 20, 30});
}

TEST_CASE("RingBuffer tracks timestamps and finds valid indices")
{
    RingBuffer buffer(3, sizeof(int));

    buffer.push(100);
    REQUIRE(buffer.get_time(buffer.head) == 100);

    auto idx_200 = buffer.push(200);
    REQUIRE(buffer.get_time(buffer.head) == 200);

    auto idx3 = buffer.push(300);
    REQUIRE(buffer.get_time(buffer.head) == 300);

    size_t index_for_200;
    REQUIRE(buffer.find_index(200, index_for_200) == true);
    REQUIRE(index_for_200 == idx_200);
    REQUIRE(buffer.get_time(index_for_200) == 200);
    
    size_t index;
    REQUIRE(buffer.find_latest_valid_index(250, index) == true);
    REQUIRE(index == index_for_200);
    REQUIRE(buffer.find_latest_valid_index(50, index) == false);

    buffer.push(400); // overwrites oldest timestamp
    REQUIRE(buffer.find_index(100, index) == false);
    REQUIRE(buffer.find_latest_valid_index(100, index) == false);
    REQUIRE(buffer.find_latest_valid_index(150, index) == false);
    REQUIRE(buffer.find_latest_valid_index(350, index) == true);
    REQUIRE(index == idx3);

    buffer.push(500); // overwrites oldest timestamp

    REQUIRE(buffer.find_latest_valid_index(100, index) == false);
    REQUIRE(buffer.find_latest_valid_index(299, index) == false);
    REQUIRE(buffer.find_latest_valid_index(300, index) == true);
    REQUIRE(buffer.find_latest_valid_index(301, index) == true);

}
