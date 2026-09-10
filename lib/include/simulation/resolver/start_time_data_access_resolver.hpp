#pragma once

#include "resolver/data_access_resolver.hpp"

namespace ssp4sim::scheduling
{
    /// Perfectly parallel (Jacobi) factory: every wired edge is stamped
    /// AccessMode::StartTime and samples the producer at step_start (shifted by
    /// the wire delay, frontier-clamped). This matches the sampling the graph
    /// builder historically pinned for Jacobi scheduling.
    class StartTimeDataAccessResolver final : public DataAccessResolver
    {
    public:
        StartTimeDataAccessResolver(std::vector<Invocable *> nodes);

    protected:
        ResolvedRead resolve(std::size_t model_id,
                             std::size_t connection_id,
                             std::uint64_t step_start,
                             std::uint64_t step_end) override;
    };
}