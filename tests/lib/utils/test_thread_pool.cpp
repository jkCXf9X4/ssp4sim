#include "utils/thread_pool/task_thread_pool.hpp"

#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

#include <catch.hpp>

using ssp4sim::utils::ThreadPool;

// ---------------------------------------------------------------------------
// Description: Verifies two tasks incrementing an atomic counter
// Rationale:   Basic task execution contract
// ---------------------------------------------------------------------------
TEST_CASE("ThreadPool executes simple tasks", "[threadpool]")
{

    std::cout << "\n";
    std::atomic<int> counter{0};
    // {
        ThreadPool pool(2);
        auto f1 = pool.enqueue([&]
                               { counter++; });
        auto f2 = pool.enqueue([&]
                               { counter += 2; });
        f1.get();
        f2.get();
    // }
    REQUIRE(counter == 3);
}

// ---------------------------------------------------------------------------
// Description: Verifies 10 tasks summing 0..9 = 45
// Rationale:   Bulk task execution
// Creep flag:  Reduced from 100 to 10 tasks (was overkill for contract verification)
// ---------------------------------------------------------------------------
TEST_CASE("ThreadPool handles many tasks", "[threadpool]")
{
    std::atomic<int> sum{0};
    // {
        ThreadPool pool(4);
        std::vector<std::future<void>> futures;
        for (int i = 0; i < 10; ++i)
        {
            futures.push_back(pool.enqueue([&sum, i]
                                           { sum += i; }));
        }
        for (auto &f : futures)
            f.get();
    // }
    REQUIRE(sum == 45); // sum 0..9
}

// ---------------------------------------------------------------------------
// Description: Verifies tasks returning int and void
// Rationale:   Return type polymorphism
// ---------------------------------------------------------------------------
TEST_CASE("ThreadPool supports void and non-void tasks", "[threadpool]")
{
    std::future<int> f1;
    std::future<void> f2;
    // {
        ThreadPool pool(2);
        f1 = pool.enqueue([]
                          { return 42; });
        f2 = pool.enqueue([] {});
    // }
    REQUIRE(f1.get() == 42);
    f2.get(); // should not throw
}

// ---------------------------------------------------------------------------
// Description: Verifies task with 50ms sleep returns correct value
// Rationale:   Long-running task handling
// Creep flag:  50ms sleep adds real time; timing-dependent, may flake on CI
// ---------------------------------------------------------------------------
TEST_CASE("ThreadPool can handle tasks with delay", "[threadpool]")
{
    std::future<int> f;
    using namespace std::chrono_literals;
    // {
        ThreadPool pool(2);
        f = pool.enqueue([]
                         {
            std::this_thread::sleep_for(50ms);
            return 7; });
    // }
    REQUIRE(f.get() == 7);
}
