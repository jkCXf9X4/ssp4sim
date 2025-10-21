#pragma once

#include "utils/log.hpp"
#include "utils/time.hpp"

#include <chrono>
#include <string>

/**
 * @brief Time related utility constants.
 */
namespace ssp4sim::utils::time
{
    class ScopeTimer
    {
    public:
        using clock = std::chrono::steady_clock;

        common::Logger log = common::Logger("ssp4cpp.common.ScopeTimer", common::LogLevel::debug);

        ScopeTimer(std::string label) : label_(label), start_(clock::now()) {}

        ScopeTimer(std::string label, uint64_t *result_callback) : ScopeTimer(label)
        {
            result_callback_ns = result_callback;
        }

        ~ScopeTimer()
        {
            const auto dur = std::chrono::duration_cast<std::chrono::nanoseconds>(clock::now() - start_);

            // log: "<func> <label> took X µs"
            if (result_callback_ns == nullptr)
            {
                log.info("[ScopeTimer] {} took {}ns", label_, (double)dur.count() / ssp4sim::utils::time::nanoseconds_per_second);
            }
            else
            {
                *result_callback_ns = dur.count();
            }
        }

    private:
        std::string label_;
        clock::time_point start_;
        uint64_t *result_callback_ns = nullptr;
    };

    class Timer
    {
    public:
        using clock = std::chrono::steady_clock;

        // common::Logger log = common::Logger("Timer", common::LogLevel::debug);

        Timer() : start_(clock::now()) {}

        uint64_t stop()
        {
            const auto dur = std::chrono::duration_cast<std::chrono::nanoseconds>(clock::now() - start_);
            return dur.count();
        }

        ~Timer()
        {
        }

    private:
        clock::time_point start_;
    };
}
