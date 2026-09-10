#include "resolver/end_time_data_access_resolver.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace ssp4sim::scheduling
{
    EndTimeDataAccessResolver::EndTimeDataAccessResolver(std::vector<Invocable *> nodes)
        : DataAccessResolver(std::move(nodes), AccessMode::EndTime)
    {
    }

    // ------------------------------------------------------------------
    // EndTime — sample the producer at step_end (Seidel friendly): a consumer may
    // use data produced earlier in the same sweep. Wire facts come from the edge's
    // transparent contract.
    // ------------------------------------------------------------------
    ResolvedRead EndTimeDataAccessResolver::resolve(std::size_t model_id,
                                                     std::size_t connection_id,
                                                     std::uint64_t,
                                                     std::uint64_t step_end)
    {
        const auto e = edge(model_id, connection_id);
        return detail::resolve_time(static_cast<std::int64_t>(step_end), *e.rules, *e.status);
    }
}