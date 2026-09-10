#pragma once

#include "resolver/data_access_resolver.hpp"

namespace ssp4sim::scheduling
{
    /// Mixed-policy resolver for the sequential-DAG + parallel-loop scheduler
    /// (La2):
    ///   - cross-SCC edges (the sequential DAG, Gauss-Seidel semantics) read
    ///     AccessMode::Latest — every consumer sees the most recently committed
    ///     full-step value of upstream components;
    ///   - intra-SCC edges (models relaxed in parallel over sub-steps) read
    ///     AccessMode::StartTime — the current sub-step start, so each sub-step
    ///     reads the previous sub-step's commitments (deterministic Jacobi
    ///     relaxation inside the loop group).
    /// Unlinked edges stay Latest (stale-only, invalid).
    class La2DataAccessResolver final : public DataAccessResolver
    {
    public:
        /// `sccs` is the strongly-connected-component partition computed by the
        /// scheduler (La2Scheduler::sccs): node groups that run in parallel.
        La2DataAccessResolver(std::vector<Invocable *> nodes,
                              std::vector<std::vector<Invocable *>> sccs);

    protected:
        ResolvedRead resolve(std::size_t model_id,
                             std::size_t connection_id,
                             std::uint64_t step_start,
                             std::uint64_t step_end) override;
    };
}