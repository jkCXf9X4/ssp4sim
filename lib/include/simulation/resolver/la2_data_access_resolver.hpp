#pragma once

#include "resolver/data_access_resolver.hpp"

#include <cstddef>
#include <vector>

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
    ///
    /// Construction-time only: this resolver derives its per-edge stamping from
    /// the SCC partition and then inherits the base resolve() (the shared
    /// per-mode dispatch in resolver_common), so it carries no resolve() override.
    class La2DataAccessResolver final : public DataAccessResolver
    {
    public:
        /// `scc_of` maps Node id -> SCC index, as computed by the scheduler
        /// from the GraphAnalysis partition. Edges whose producer and consumer
        /// share an SCC are intra-SCC (StartTime); everything else keeps the
        /// Latest default (cross-SCC / unlinked).
        La2DataAccessResolver(std::vector<Invocable *> nodes,
                              std::vector<std::size_t> scc_of);
    };
}