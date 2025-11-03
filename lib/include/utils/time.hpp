#pragma once

#include <chrono>
#include <format>
#include <string>
#include <iostream>

/**
 * @brief Time related utility constants.
 */
namespace ssp4sim::utils::time
{
    uint64_t constexpr milliseconds_per_second = 1000;
    uint64_t constexpr microseconds_per_second = 1000000;
    uint64_t constexpr nanoseconds_per_second = 1000000000;

    uint64_t constexpr nanoseconds_per_millisecond = 1000000;
    uint64_t constexpr nanoseconds_per_microsecond = 1000;

    uint64_t constexpr seconds_per_hour = 3600;
    uint64_t constexpr hours_per_day = 24;

    double ns_to_s(uint64_t t);

    uint64_t s_to_ns(double t);

    std::chrono::system_clock::time_point time_now();

    std::string time_now_str();
}
