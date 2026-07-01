#include <catch.hpp>

#include "simulation/signal/mpsc_event_queue.hpp"

#include <algorithm>
#include <atomic>
#include <thread>
#include <vector>

using ssp4sim::signal::BoundedMpscEventQueue;

TEST_CASE("BoundedMpscEventQueue reports full without overwriting unread events", "[BoundedMpscEventQueue]")
{
    BoundedMpscEventQueue<int> queue(2);

    REQUIRE(queue.try_push(1));
    REQUIRE(queue.try_push(2));
    REQUIRE_FALSE(queue.try_push(3));

    int value = 0;
    REQUIRE(queue.try_pop(value));
    REQUIRE(value == 1);
    REQUIRE(queue.try_push(3));
    REQUIRE(queue.try_pop(value));
    REQUIRE(value == 2);
    REQUIRE(queue.try_pop(value));
    REQUIRE(value == 3);
    REQUIRE_FALSE(queue.try_pop(value));
}

TEST_CASE("BoundedMpscEventQueue accepts events from multiple producers and one consumer", "[BoundedMpscEventQueue]")
{
    constexpr int producer_count = 4;
    constexpr int events_per_producer = 2000;
    constexpr int total_events = producer_count * events_per_producer;

    BoundedMpscEventQueue<int> queue(256);
    std::atomic<int> producers_done = 0;
    std::vector<std::thread> producers;
    producers.reserve(producer_count);

    for (int producer = 0; producer < producer_count; ++producer)
    {
        producers.emplace_back([producer, &queue, &producers_done]()
                               {
                                   const auto base = producer * events_per_producer;
                                   for (int offset = 0; offset < events_per_producer; ++offset)
                                   {
                                       while (!queue.try_push(base + offset))
                                       {
                                           std::this_thread::yield();
                                       }
                                   }
                                   producers_done.fetch_add(1, std::memory_order_release);
                               });
    }

    std::vector<int> received;
    received.reserve(total_events);
    while (producers_done.load(std::memory_order_acquire) != producer_count || received.size() < total_events)
    {
        int value = 0;
        if (queue.try_pop(value))
        {
            received.push_back(value);
        }
        else
        {
            std::this_thread::yield();
        }
    }

    for (auto &producer : producers)
    {
        producer.join();
    }

    std::sort(received.begin(), received.end());
    REQUIRE(received.size() == total_events);
    for (int expected = 0; expected < total_events; ++expected)
    {
        REQUIRE(received[expected] == expected);
    }
}
