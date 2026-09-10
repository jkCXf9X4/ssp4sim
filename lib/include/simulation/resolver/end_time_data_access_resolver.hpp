#pragma once

#include "resolver/data_access_resolver.hpp"

namespace ssp4sim::scheduling
{
    /// Seidel-friendly factory: every wired edge is stamped AccessMode::EndTime
    /// and samples the producer at step_end (shifted by the wire delay,
    /// frontier-clamped), letting a consumer use data produced earlier in the
    /// same sweep.
    class EndTimeDataAccessResolver final : public DataAccessResolver
    {
    public:
        EndTimeDataAccessResolver(std::vector<Invocable *> nodes);

    protected:
        ResolvedRead resolve(std::size_t model_id,
                             std::size_t connection_id,
                             std::uint64_t step_start,
                             std::uint64_t step_end) override;
    };
}