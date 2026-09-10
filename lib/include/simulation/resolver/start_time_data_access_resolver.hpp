#pragma once

#include "resolver/data_access_resolver.hpp"

namespace ssp4sim::scheduling
{
    /// Perfectly parallel (Jacobi): every wired edge samples the producer at
    /// step_start (shifted by the wire delay, frontier-clamped). This matches the
    /// sampling the graph builder historically pinned for Jacobi scheduling.
    class StartTimeDataAccessResolver final : public DataAccessResolver
    {
    public:
        StartTimeDataAccessResolver(std::vector<Invocable *> nodes);

    protected:
        ResolvedRead resolve_edge(const EdgeAccessRules &access,
                                  const detail::ModelStatus &status,
                                  std::uint64_t step_start,
                                  std::uint64_t step_end) override;
    };
}