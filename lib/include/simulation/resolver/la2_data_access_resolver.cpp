#include "resolver/la2_data_access_resolver.hpp"

#include "pre/3_simulation_graph/elements/model_fmu.hpp"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace ssp4sim::scheduling
{
    La2DataAccessResolver::La2DataAccessResolver(std::vector<Invocable *> nodes,
                                                 std::vector<std::size_t> scc_of)
        : DataAccessResolver(nodes, AccessMode::Latest) // cross-SCC default; `nodes` stays usable below
    {
        // Stamp every wired edge explicitly: intra-SCC edges StartTime, cross-SCC
        // edges Latest. Do not rely on the base constructor's default mode staying
        // Latest if this policy set ever changes. The base already knows each
        // edge's source producer, so scc_of (node id -> SCC index) and the
        // protected edge_source_producer() accessor are enough — no need to
        // re-derive storage ownership here.
        for (auto *node : nodes)
        {
            auto *fmu = dynamic_cast<ssp4sim::graph::FmuModel *>(node);
            if (fmu == nullptr)
            {
                continue;
            }

            const std::size_t target_scc = (static_cast<std::size_t>(fmu->id) < scc_of.size())
                ? scc_of[static_cast<std::size_t>(fmu->id)] : no_producer;
            if (target_scc == no_producer)
            {
                continue;
            }

            for (std::size_t i = 0; i < fmu->connections.size(); ++i)
            {
                const std::size_t src = edge_source_producer(static_cast<std::size_t>(fmu->id), i);
                if (src == no_producer)
                {
                    continue; // unlinked (uc-14): stays Latest, never valid
                }

                const std::size_t src_scc = (src < scc_of.size()) ? scc_of[src] : no_producer;
                if (src_scc == target_scc)
                {
                    // Same SCC -> the producer relaxes in parallel with this model;
                    // sample at the sub-step start for determinism.
                    stamp_edge_mode(static_cast<std::size_t>(fmu->id), i, AccessMode::StartTime);
                }
                else
                {
                    // Cross-SCC edge -> Gauss-Seidel: zero-order-hold the producer's
                    // latest committed value. Explicit so the mode doesn't silently
                    // ride on the base constructor's default.
                    stamp_edge_mode(static_cast<std::size_t>(fmu->id), i, AccessMode::Latest);
                }
            }
        }
    }
}
