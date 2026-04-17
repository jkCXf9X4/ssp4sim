#pragma once

#include "ssp4cpp/utils/log.hpp"
#include "utils/time.hpp"

#include <chrono>
#include <cstdint>
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

        ScopeTimer(std::string label, uint64_t *result_callback, quill::Logger *log);

        ~ScopeTimer();

    private:
        std::string label_;
        clock::time_point start_;
        uint64_t *result_callback_ns = nullptr;
        quill::Logger *log;
    };

    class Timer
    {
    public:
        using clock = std::chrono::steady_clock;

        // quill::Logger* log = nullptr;

        Timer();

        uint64_t stop();

        ~Timer();

    private:
        clock::time_point start_;
    };
}
