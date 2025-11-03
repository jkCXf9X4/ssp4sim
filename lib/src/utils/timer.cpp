#include "utils/timer.hpp"

#include "utils/time.hpp"

#include <chrono>
#include <string>
#include <utility>

namespace ssp4sim::utils::time
{

    ScopeTimer::ScopeTimer(std::string label) : label_(std::move(label)), start_(clock::now()) {}

    ScopeTimer::ScopeTimer(std::string label, uint64_t *result_callback) : ScopeTimer(std::move(label))
    {
        result_callback_ns = result_callback;
    }

    ScopeTimer::~ScopeTimer()
    {
        const auto dur = std::chrono::duration_cast<std::chrono::nanoseconds>(clock::now() - start_);

        if (result_callback_ns == nullptr)
        {
            log(info)("[ScopeTimer] {} took {}ns", label_, (double)dur.count() / ssp4sim::utils::time::nanoseconds_per_second);
        }
        else
        {
            *result_callback_ns = static_cast<uint64_t>(dur.count());
        }
    }

    Timer::Timer() : start_(clock::now()) {}

    uint64_t Timer::stop()
    {
        const auto dur = std::chrono::duration_cast<std::chrono::nanoseconds>(clock::now() - start_);
        return static_cast<uint64_t>(dur.count());
    }

    Timer::~Timer() = default;

}
