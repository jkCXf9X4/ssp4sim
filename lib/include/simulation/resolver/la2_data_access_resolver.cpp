#include "resolver/la2_data_access_resolver.hpp"

#include "pre/3_simulation_graph/elements/model_fmu.hpp"
#include "signal/storage.hpp"

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ssp4sim::scheduling
{
    La2DataAccessResolver::La2DataAccessResolver(std::vector<Invocable *> nodes,
                                                 std::vector<std::vector<Invocable *>> sccs)
        : DataAccessResolver(nodes, AccessMode::Latest) // pass by value; `nodes` stays usable below
    {
        // node id -> SCC index, so we can tell whether an edge stays inside the
        // same parallel group.
        std::unordered_map<std::size_t, std::size_t> scc_of;
        for (std::size_t si = 0; si < sccs.size(); ++si)
        {
            for (auto *node : sccs[si])
            {
                scc_of.emplace(static_cast<std::size_t>(node->id), si);
            }
        }

        // output storage -> owning node (mirrors the base's ownership pass).
        std::unordered_map<ssp4sim::signal::SignalStorage *, Invocable *> owner;
        for (auto *node : nodes)
        {
            auto *fmu = dynamic_cast<ssp4sim::graph::FmuModel *>(node);
            if (fmu == nullptr)
            {
                continue;
            }
            owner.emplace(fmu->output_area.get(), node);
        }

        // Stamp intra-SCC edges StartTime; everything else keeps the Latest default.
        for (auto *node : nodes)
        {
            auto *fmu = dynamic_cast<ssp4sim::graph::FmuModel *>(node);
            if (fmu == nullptr)
            {
                continue;
            }

            auto target_it = scc_of.find(static_cast<std::size_t>(fmu->id));
            if (target_it == scc_of.end())
            {
                continue;
            }

            for (std::size_t i = 0; i < fmu->connections.size(); ++i)
            {
                auto src_it = owner.find(fmu->connections[i].source_storage);
                if (src_it == owner.end())
                {
                    continue; // unlinked (uc-14): stays Latest, never valid
                }

                auto src_scc_it = scc_of.find(static_cast<std::size_t>(src_it->second->id));
                if (src_scc_it != scc_of.end() && src_scc_it->second == target_it->second)
                {
                    // Same SCC -> the producer relaxes in parallel with this model;
                    // sample at the sub-step start for determinism.
                    stamp_edge_mode(static_cast<std::size_t>(fmu->id), i, AccessMode::StartTime);
                }
            }
        }
    }

    // ------------------------------------------------------------------
    // La2 resolve — per (model, connection): the stamped per-edge mode decides,
    // so intra-SCC edges sample the current sub-step (StartTime) and cross-SCC
    // edges zero-order-hold the producer's latest commit (Latest / Gauss-Seidel).
    // The shared dispatcher does the rest.
    // ------------------------------------------------------------------
    ResolvedRead La2DataAccessResolver::resolve(std::size_t model_id,
                                                std::size_t connection_id,
                                                std::uint64_t step_start,
                                                std::uint64_t step_end)
    {
        return detail::resolve_edge(edge_rules(model_id, connection_id),
                                    producer_status(model_id, connection_id),
                                    step_start, step_end);
    }
}