// Scheduling-behavior tests for the La2Scheduler stack after the nested-executor
// refactor:
//  - acyclic SCCs (bare models) run once per macro step; only loop SCCs are
//    sub-stepped;
//  - the outer Gauss-Seidel walk respects the condensed component order;
//  - the ParallelSeidel seam fails loudly while the stub is in place.

#include <catch2/catch_test_macros.hpp>

#include "config.hpp"
#include "executor/loop_aware/la2_scheduler.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using ssp4sim::utils::Config;

namespace
{
    // Invocable that appends its name to a shared log on every invocation.
    class LoggingNode final : public ssp4sim::graph::Invocable
    {
    public:
        LoggingNode(std::string name, std::vector<std::string> *log)
            : log(log)
        {
            this->name = std::move(name);
        }

        std::uint64_t invoke(ssp4sim::graph::StepData) override
        {
            log->push_back(name);
            return 0;
        }

        std::vector<std::string> *log = nullptr;
    };

    std::vector<std::shared_ptr<ssp4sim::graph::Invocable>> to_owned(
        std::vector<std::shared_ptr<LoggingNode>> storage)
    {
        return std::vector<std::shared_ptr<ssp4sim::graph::Invocable>>(
            storage.begin(), storage.end());
    }

    // A -> B -> C -> B (loop {B,C}); C -> D.
    std::vector<std::shared_ptr<LoggingNode>> make_loop_with_tails(
        std::vector<std::string> *log)
    {
        auto a = std::make_shared<LoggingNode>("A", log);
        auto b = std::make_shared<LoggingNode>("B", log);
        auto c = std::make_shared<LoggingNode>("C", log);
        auto d = std::make_shared<LoggingNode>("D", log);
        a->add_child(b.get());
        b->add_child(c.get());
        c->add_child(b.get());
        c->add_child(d.get());
        return {std::move(a), std::move(b), std::move(c), std::move(d)};
    }

    constexpr std::uint64_t T0 = 0;
    constexpr std::uint64_t T1 = 1200;
} // namespace

TEST_CASE("la2 sub-steps loop SCCs only, acyclic nodes run once per macro step",
          "[la2][scheduling]")
{
    Config::loadFromString(R"json({
        "simulation": {
            "timestep": 1e-6,
            "executor": {
                "method": "la2",
                "la2": { "iterations": 2, "mode": "linear" }
            }
        }
    })json");

    std::vector<std::string> log;
    auto storage = make_loop_with_tails(&log);
    ssp4sim::graph::La2Scheduler scheduler(to_owned(storage));
    scheduler.invoke(ssp4sim::graph::StepData(T0, T1));

    auto count = [&](const std::string &name)
    {
        return static_cast<std::size_t>(
            std::count(log.begin(), log.end(), name));
    };
    // Acyclic head A and tail D run exactly once; loop members B, C relax over
    // the 2 configured sub-steps each.
    REQUIRE(count("A") == 1);
    REQUIRE(count("B") == 2);
    REQUIRE(count("C") == 2);
    REQUIRE(count("D") == 1);
    REQUIRE(log.size() == 6);

    // Gauss-Seidel cascade: A runs before the loop, D after the loop's last
    // sub-step.
    auto first = [&](const std::string &name)
    {
        return static_cast<std::size_t>(
            std::find(log.begin(), log.end(), name) - log.begin());
    };
    auto last = [&](const std::string &name)
    {
        auto it = std::find(log.rbegin(), log.rend(), name);
        return static_cast<std::size_t>((log.rend() - it) - 1);
    };
    REQUIRE(first("A") < first("B"));
    REQUIRE(last("C") < first("D"));
}

TEST_CASE("la2.parallel fails loudly until ParallelSeidel is implemented",
          "[la2][config]")
{
    Config::loadFromString(R"json({
        "simulation": {
            "timestep": 1e-6,
            "executor": {
                "method": "la2",
                "la2": { "parallel": true }
            }
        }
    })json");

    std::vector<std::string> log;
    auto storage = make_loop_with_tails(&log);

    auto build = [&]()
    {
        ssp4sim::graph::La2Scheduler scheduler(to_owned(storage));
    };
    REQUIRE_THROWS_AS(build(), std::runtime_error);
}