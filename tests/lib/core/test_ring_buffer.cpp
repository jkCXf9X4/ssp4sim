#include "utils/ring_buffer.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <array>
#include <stdexcept>

using ssp4sim::utils::RingBuffer;

TEST_CASE("RingBuffer rejects zero capacity", "[RingBuffer]")
{
    REQUIRE_THROWS_AS(RingBuffer(0, sizeof(std::uint32_t)), std::runtime_error);
}

TEST_CASE("RingBuffer push grows until full then overwrites oldest", "[RingBuffer]")
{
    constexpr std::size_t kCapacity = 3;
    RingBuffer buffer(kCapacity, sizeof(std::uint32_t));

    REQUIRE(buffer.is_empty());
    REQUIRE_FALSE(buffer.is_full());
    REQUIRE(buffer.head == 0);

    // first
    auto expected_head = 1;
    auto first_index = buffer.push();
    REQUIRE(first_index == expected_head);
    REQUIRE(buffer.head == expected_head);
    REQUIRE(buffer.nr_inserts == 1);
    REQUIRE_FALSE(buffer.is_empty());

    // second
    expected_head = 2;
    auto second_index = buffer.push();
    REQUIRE(second_index == expected_head);
    REQUIRE(buffer.head == expected_head);
    REQUIRE(buffer.nr_inserts == 2);

    // 3 wraparound
    expected_head = 0;
    auto third_index = buffer.push();
    REQUIRE(third_index == expected_head);
    REQUIRE(buffer.head == expected_head);
    REQUIRE(buffer.nr_inserts == 3);
    REQUIRE(buffer.is_full());

    // 4 overwrite 
    expected_head = 1;
    auto fourth_index = buffer.push();
    REQUIRE(fourth_index == expected_head);
    REQUIRE(buffer.nr_inserts == 4);
}

TEST_CASE("RingBuffer get_item provides contiguous storage and bounds checks", "[RingBuffer]")
{
    constexpr std::size_t element_size = sizeof(std::uint64_t);
    RingBuffer buffer(3, element_size);

    auto idx1 = buffer.push();
    // should 
    buffer.get_item(idx1);
    REQUIRE_THROWS_AS(buffer.get_item(idx1+1), std::runtime_error); // ensure that its not out of bounds

    auto idx2 = buffer.push();

    auto *first = buffer.get_item(idx1);
    auto *second = buffer.get_item(idx2);
    REQUIRE(second - first == static_cast<std::ptrdiff_t>(element_size));

    *reinterpret_cast<std::uint64_t *>(first) = 42;
    *reinterpret_cast<std::uint64_t *>(second) = 99;
    REQUIRE(*reinterpret_cast<std::uint64_t *>(buffer.get_item(idx1)) == 42);
    REQUIRE(*reinterpret_cast<std::uint64_t *>(buffer.get_item(idx2)) == 99);

    auto idx3 = buffer.push();

    // access outside bounds when full
    REQUIRE_THROWS_AS(buffer.get_item(buffer.capacity), std::runtime_error);
}

TEST_CASE("RingBuffer index helpers follow head/tail ordering", "[RingBuffer]")
{
    RingBuffer buffer(3, sizeof(int));
    

    auto idx1 = buffer.push();
    *reinterpret_cast<int *>(buffer.get_item(idx1)) = 10;
    // ensure this works while buffer is not full
    auto index = buffer.get_index_from_pos_rev(0);
    REQUIRE(index == idx1);
    REQUIRE(*reinterpret_cast<int *>(buffer.get_item(index)) == 10);
    
    auto idx2 = buffer.push();
    *reinterpret_cast<int *>(buffer.get_item(idx2)) = 20;
    index = buffer.get_index_from_pos_rev(1);
    REQUIRE(index == idx1);
    REQUIRE(*reinterpret_cast<int *>(buffer.get_item(index)) == 10);

    auto idx3 = buffer.push();
    *reinterpret_cast<int *>(buffer.get_item(idx3)) = 30;
    REQUIRE(buffer.is_full());

    // Reverse lookup should walk newest to oldest.
    REQUIRE(*reinterpret_cast<int *>(buffer.get_item(buffer.get_index_from_pos_rev(0))) == 30);
    REQUIRE(*reinterpret_cast<int *>(buffer.get_item(buffer.get_index_from_pos_rev(1))) == 20);
    REQUIRE(*reinterpret_cast<int *>(buffer.get_item(buffer.get_index_from_pos_rev(2))) == 10);

    // Overwrite oldest and confirm head/tail advance and value mapping update.
    auto idx4 = buffer.push();
    *reinterpret_cast<int *>(buffer.get_item(idx4)) = 40;

    REQUIRE(buffer.is_full());

    REQUIRE(*reinterpret_cast<int *>(buffer.get_item(buffer.get_index_from_pos_rev(0))) == 40);
    REQUIRE(*reinterpret_cast<int *>(buffer.get_item(buffer.get_index_from_pos_rev(1))) == 30);
    REQUIRE(*reinterpret_cast<int *>(buffer.get_item(buffer.get_index_from_pos_rev(2))) == 20);
}

TEST_CASE("RingBuffer tracks timestamps and finds valid indices", "[RingBuffer]")
{
    RingBuffer buffer(3, sizeof(int));

    buffer.push(100);
    REQUIRE(buffer.get_time(buffer.head) == 100);

    auto idx_200 = buffer.push(200);
    REQUIRE(buffer.get_time(buffer.head) == 200);
    // ensure these work while buffer is not full
    size_t index_for_200;
    REQUIRE(buffer.find_index(200, index_for_200) == true);
    REQUIRE(index_for_200 == idx_200);
    size_t index;
    REQUIRE(buffer.find_latest_valid_index(250, index) == true);
    REQUIRE(index == idx_200);

    auto idx3 = buffer.push(300);
    REQUIRE(buffer.get_time(buffer.head) == 300);

    REQUIRE(buffer.find_index(200, index_for_200) == true);
    REQUIRE(index_for_200 == idx_200);
    REQUIRE(buffer.get_time(index_for_200) == 200);

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
