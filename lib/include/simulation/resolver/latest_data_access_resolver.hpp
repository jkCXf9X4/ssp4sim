#pragma once

#include "resolver/data_access_resolver.hpp"

namespace ssp4sim::scheduling
{
    /// Zero-order hold factory: every wired edge is stamped AccessMode::Latest and
    /// reads the newest committed area of its producer. step_start / step_end (and
    /// the wire delay) are never consulted.
    class LatestDataAccessResolver final : public DataAccessResolver
    {
    public:
        LatestDataAccessResolver(std::vector<Invocable *> nodes);

    protected:
        ResolvedRead resolve(std::size_t model_id,
                             std::size_t connection_id,
                             std::uint64_t step_start,
                             std::uint64_t step_end) override;
    };
}