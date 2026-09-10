#pragma once

#include "resolver/data_access_resolver.hpp"

namespace ssp4sim::scheduling
{
    /// Zero-order hold: every wired edge reads the newest committed area of its
    /// producer. step_start / step_end (and the wire delay) are never consulted.
    class LatestDataAccessResolver final : public DataAccessResolver
    {
    public:
        LatestDataAccessResolver(std::vector<Invocable *> nodes);

    protected:
        ResolvedRead resolve_edge(const EdgeAccessRules &access,
                                  const detail::ModelStatus &status,
                                  std::uint64_t step_start,
                                  std::uint64_t step_end) override;
    };
}