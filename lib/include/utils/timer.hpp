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

        quill::Logger* log = ssp4cpp::utils::log::make_logger("ssp4cpp.common.ScopeTimer", quill::LogLevel::TraceL1);

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

        // quill::Logger* log = ssp4cpp::utils::log::make_logger("Timer", quill::LogLevel::TraceL1);

        Timer();

        uint64_t stop();

        ~Timer();

    private:
        clock::time_point start_;
    };
}
