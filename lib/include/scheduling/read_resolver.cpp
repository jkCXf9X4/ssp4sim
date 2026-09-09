#include "read_resolver.hpp"

namespace ssp4sim::scheduling
{
    void detail::commit_frontier(detail::ModelStatus &status,
                                 std::uint64_t output_time,
                                 std::size_t area)
    {
        status.committed_count.fetch_add(1, std::memory_order::release);
        status.committed_time.store(output_time, std::memory_order::release);
        status.latest_area.store(area, std::memory_order::release);
        status.generation.fetch_add(1, std::memory_order::release);
    }
}