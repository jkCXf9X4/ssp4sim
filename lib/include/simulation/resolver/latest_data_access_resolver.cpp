#include "resolver/latest_data_access_resolver.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace ssp4sim::scheduling
{
    LatestDataAccessResolver::LatestDataAccessResolver(std::vector<Invocable *> nodes)
        : DataAccessResolver(std::move(nodes), AccessMode::Latest)
    {
    }

    // ------------------------------------------------------------------
    // Latest — zero-order hold on the newest committed area of the edge's
    // producer. step_start / step_end (and the wire delay) are never consulted:
    // only the producer frontier matters.
    // ------------------------------------------------------------------
    ResolvedRead LatestDataAccessResolver::resolve(std::size_t model_id,
                                                   std::size_t connection_id,
                                                   std::uint64_t,
                                                   std::uint64_t)
    {
        return detail::resolve_latest(producer_status(model_id, connection_id));
    }
}