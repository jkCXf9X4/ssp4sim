#pragma once

#include "cutecpp/log.hpp"
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

        Logger log = Logger("ssp4cpp.common.ScopeTimer", LogLevel::debug);

        explicit ScopeTimer(std::string label);

        ScopeTimer(std::string label, uint64_t *result_callback);

        ~ScopeTimer();

    private:
        std::string label_;
        clock::time_point start_;
        uint64_t *result_callback_ns = nullptr;
    };

    class Timer
    {
    public:
        using clock = std::chrono::steady_clock;

        // Logger log = Logger("Timer", LogLevel::debug);

        Timer();

        uint64_t stop();

        ~Timer();

    private:
        clock::time_point start_;
    };
}
