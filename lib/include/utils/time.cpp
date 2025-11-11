#include "utils/time.hpp"

#include <chrono>
#include <string>

#include "cutecpp/log.hpp"

namespace ssp4sim::utils::time
{

    double ns_to_s(uint64_t t)
    {
        return static_cast<double>(t) / utils::time::nanoseconds_per_second;
    }

    uint64_t s_to_ns(double t)
    {
        return static_cast<uint64_t>(t * utils::time::nanoseconds_per_second);
    }

    std::chrono::system_clock::time_point time_now()
    {
        using namespace std::chrono;
        return system_clock::now();
    }

    std::string time_now_str()
    {
        using namespace std::chrono;
        auto now = system_clock::now();
        return Logger::format("{:%Y-%m-%d_%H-%M-%S}", zoned_time{current_zone(), now});
    }

}

