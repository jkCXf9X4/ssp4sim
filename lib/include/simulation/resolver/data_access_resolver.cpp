#include "resolver/data_access_resolver.hpp"

#include "pre/3_simulation_graph/elements/model_fmu.hpp"
#include "signal/storage.hpp"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ssp4sim::scheduling
{
    // ------------------------------------------------------------------
    // Opaque implementation state of the resolver. All tables are vectors
    // indexed by the unique, 0..N-ish Invocable::id; connections are static
    // after graph build, so per-(model, connection) edge facts are a plain
    // vector-of-vectors lookup — no maps in the read path.
    // ------------------------------------------------------------------
    struct DataAccessResolver::State
    {
        // No-registered-producer sentinel.
        static constexpr std::size_t npos = std::size_t(-1);

        // One registered edge per (model, incoming connection), index-aligned with
        // model->connections so copy_model_inputs can resolve(model, i) directly.
        struct RegisteredEdge
        {
            // Transparent per-edge contract (wire facts + sampling intent); the
            // class policy in resolve_edge() is the authority on sampling.
            EdgeAccessRules access;
            std::size_t source_producer = npos; // status row; npos = unlinked
        };

        std::vector<std::vector<RegisteredEdge>> edges;                 // [model_id][connection_id]
        std::vector<std::unique_ptr<detail::ModelStatus>> status;       // [producer id], nullptr = unregistered

        // Fallback for source storages with no registered owner (unlinked, uc-14):
        // never committed, so every policy resolves them to invalid (D2/D13 gate).
        detail::ModelStatus never_committed{};
    };

    // ------------------------------------------------------------------
    // Build per-model edge rule tables from the graph nodes. Only FmuModel
    // nodes are registered: pass 1 records output-storage ownership and the
    // per-producer frontier rows, pass 2 snapshots each model's incoming
    // connections as edges in connections order. Only wire facts (delay) and
    // producer ownership are snapshotted here; the sampling policy per edge is
    // decided by the concrete subclass's resolve_edge().
    // ------------------------------------------------------------------
    DataAccessResolver::DataAccessResolver(std::vector<Invocable *> nodes)
        : s_(new DataAccessResolver::State)
    {
        std::unordered_map<ssp4sim::signal::SignalStorage *, std::size_t> owner;
        std::size_t max_id = 0;

        // Pass 1: register ownership + the status row for each producer.
        for (const auto &node : nodes)
        {
            auto *fmu = dynamic_cast<ssp4sim::graph::FmuModel *>(node);
            if (fmu == nullptr)
            {
                continue;
            }
            owner.emplace(fmu->output_area.get(), static_cast<std::size_t>(fmu->id));
            max_id = std::max(max_id, static_cast<std::size_t>(fmu->id));
        }

        s_->edges.resize(max_id + 1);
        s_->status.resize(max_id + 1);

        // Pass 2: snapshot each target model's incoming connections as edges,
        // index-aligned with model->connections.
        for (const auto &node : nodes)
        {
            auto *fmu = dynamic_cast<ssp4sim::graph::FmuModel *>(node);
            if (fmu == nullptr)
            {
                continue;
            }

            std::vector<DataAccessResolver::State::RegisteredEdge> model_edges;
            model_edges.reserve(fmu->connections.size());

            for (const ssp4sim::graph::ConnectionInfo &c : fmu->connections)
            {
                DataAccessResolver::State::RegisteredEdge e;
                e.access.delay = static_cast<std::int64_t>(c.delay);

                auto owner_it = owner.find(c.source_storage);
                if (owner_it != owner.end())
                {
                    // Wired edge: the graph guarantees the source is one of the
                    // registered producers; the subclass supplies the sampling policy.
                    e.source_producer = owner_it->second;
                }
                else
                {
                    // Unlinked (uc-14): no registered owner, hence no schedule
                    // happens-before. Stamped Latest so every policy sees the stale-only
                    // intent; it also resolves against never_committed and is invalid.
                    e.access.mode = AccessMode::Latest;
                }

                model_edges.push_back(std::move(e));
            }

            s_->edges[fmu->id] = std::move(model_edges);
            s_->status[fmu->id] = std::make_unique<detail::ModelStatus>();
        }
    }

    DataAccessResolver::~DataAccessResolver() noexcept
    {
        delete s_;
        s_ = nullptr;
    }

    // ------------------------------------------------------------------
    // Advance one producer's committed frontier. Called ONLY after the producer's
    // output bytes are fully visible (D17). No-op for producers the resolver has
    // not registered.
    // ------------------------------------------------------------------
    void DataAccessResolver::mark_committed(std::size_t model_id,
                                            std::uint64_t output_time,
                                            std::size_t area)
    {
        if (s_ == nullptr || model_id >= s_->status.size())
        {
            return;
        }

        detail::ModelStatus *frontier = s_->status[model_id].get();
        if (frontier == nullptr)
        {
            return;
        }

        // D17: publish the new frontier with release-store ordering and the
        // committed_count gate LAST, so a reader that acquires on the count sees
        // the whole frontier.
        frontier->committed_time.store(output_time, std::memory_order::release);
        frontier->latest_area.store(area, std::memory_order::release);
        frontier->committed_count.fetch_add(1, std::memory_order::release);
    }

    // ------------------------------------------------------------------
    // Pure resolution: "what to read" for one edge under one producer frontier.
    // No storage, no I/O, no mutation. Dispatches to the concrete subclass's
    // resolve_edge(), which is the specialized access policy. Invalid for unknown
    // model/connection or a producer that has not committed yet (D2/D13).
    // ------------------------------------------------------------------
    ResolvedRead DataAccessResolver::resolve(std::size_t model_id,
                                             std::size_t connection_id,
                                             std::uint64_t step_start,
                                             std::uint64_t step_end)
    {
        ResolvedRead r{};

        if (s_ == nullptr || model_id >= s_->edges.size())
        {
            return r; // unknown model
        }

        const std::vector<DataAccessResolver::State::RegisteredEdge> &model_edges = s_->edges[model_id];
        if (connection_id >= model_edges.size())
        {
            return r; // unknown connection
        }

        const DataAccessResolver::State::RegisteredEdge &e = model_edges[connection_id];

        // Frontier of the source producer; a producer without a registered status
        // (unlinked, uc-14) behaves as never committed (D2/D13 gate).
        const detail::ModelStatus *frontier = &s_->never_committed;
        if (e.source_producer != DataAccessResolver::State::npos &&
            e.source_producer < s_->status.size() &&
            s_->status[e.source_producer] != nullptr)
        {
            frontier = s_->status[e.source_producer].get();
        }

        return resolve_edge(e.access, *frontier, step_start, step_end);
    }

    void DataAccessResolver::copy_model_inputs(ssp4sim::graph::FmuModel *target,
                                               std::size_t target_area,
                                               std::uint64_t step_start,
                                               std::uint64_t step_end)
    {
        if (s_ == nullptr || target == nullptr)
        {
            return;
        }

        const std::vector<ssp4sim::graph::ConnectionInfo> &connections = target->connections;

        for (std::size_t i = 0; i < connections.size(); ++i)
        {
            // Intentionally cheap per connection; the resolver's own edges are
            // index-aligned with target->connections.
            const ResolvedRead r = resolve(target->id, i, step_start, step_end);
            detail::copy_connection(connections[i], target_area, r);
        }
    }

}