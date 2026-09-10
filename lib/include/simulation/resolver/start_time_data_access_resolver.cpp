#include "resolver/start_time_data_access_resolver.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace ssp4sim::scheduling
{
    StartTimeDataAccessResolver::StartTimeDataAccessResolver(std::vector<Invocable *> nodes)
        : DataAccessResolver(std::move(nodes), AccessMode::StartTime)
    {
    }

    // ------------------------------------------------------------------
    // StartTime — sample the producer at step_start (perfectly parallel /
    // Jacobi): every model reads the inputs produced in the previous step. The
    // wire facts (delay/time_offset) come from the edge's transparent contract.
    // ------------------------------------------------------------------
    ResolvedRead StartTimeDataAccessResolver::resolve(std::size_t model_id,
                                                       std::size_t connection_id,
                                                       std::uint64_t step_start,
                                                       std::uint64_t)
    {
        const auto e = edge(model_id, connection_id);
        return detail::resolve_time(static_cast<std::int64_t>(step_start), *e.rules, *e.status);
    }
}