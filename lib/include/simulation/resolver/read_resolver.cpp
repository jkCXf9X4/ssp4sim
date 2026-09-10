#include "read_resolver.hpp"

#include "read_target_core.hpp"

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
    // detail::resolve_edge — the pure resolution core, shared with the
    // resolver and directly with tests. Reduces (EdgeAccessRules, one producer
    // frontier) to "an area index" (Latest / Index) or "a viable, frontier-clamped
    // reference time" (StartTime / EndTime). No storage, no I/O, no mutation.
    // ------------------------------------------------------------------
    ResolvedRead detail::resolve_edge(const EdgeAccessRules &access,
                                      const detail::ModelStatus &status,
                                      std::uint64_t step_start,
                                      std::uint64_t step_end)
    {
        ResolvedRead r{};

        const std::uint64_t committed_count = status.committed_count.load(std::memory_order::acquire);

        // Latest: newest committed area index, zero-order hold, no time involved.
        // Unlinked sources arrive here too; with no registered producer committing
        // to them they resolve against the never-committed fallback status and are
        // invalid (D2/D13-safe, keep init).
        if (access.mode == AccessMode::Latest)
        {
            if (committed_count == 0)
            {
                return r;
            }
            r.valid = true;
            r.is_area = true;
            r.area = status.latest_area.load(std::memory_order::acquire);
            r.write_counter = committed_count;
            return r;
        }

        // Index: absolute fixed physical slot, no time involved. The caller applies
        // the populated gate, which needs the storage (unreachable from here).
        if (access.mode == AccessMode::Index)
        {
            r.valid = true;
            r.is_area = true;
            r.area = static_cast<std::size_t>(access.fixed_index);
            r.write_counter = committed_count;
            return r;
        }

        // StartTime / EndTime: the step handle the connection samples at, shifted by
        // time_offset and delay.
        std::uint64_t base;
        switch (access.mode)
        {
            case AccessMode::StartTime:
                base = step_start;
                break;
            case AccessMode::EndTime:
            default:
                base = step_end;
                break;
        }

        // Producer has never committed (first-commit / t=0) — no valid data (D2/D13).
        if (committed_count == 0)
        {
            return r;
        }

        std::int64_t ref = static_cast<std::int64_t>(base) + access.time_offset - access.delay;
        // Never read past the committed frontier (M1a); floor at 0 (D8).
        ref = std::min(ref, static_cast<std::int64_t>(status.committed_time.load(std::memory_order::acquire)));
        if (ref < 0)
        {
            ref = 0;
        }

        r.valid = true;
        r.is_area = false;
        r.time = static_cast<std::uint64_t>(ref);
        r.write_counter = committed_count;
        return r;
    }

    // ------------------------------------------------------------------
    // Opaque implementation state of the resolver. All tables are vectors
    // indexed by the unique, 0..N-ish Invocable::id; connections are static
    // after graph build, so per-(model, connection) edge rules are a plain
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
            EdgeAccessRules access;
            ssp4sim::signal::SignalStorage *source = nullptr; // for the Index populated gate
            std::size_t source_producer = npos;               // status row; npos = unlinked
        };

        std::vector<std::vector<RegisteredEdge>> edges;                 // [model_id][connection_id]
        std::vector<std::unique_ptr<detail::ModelStatus>> status;       // [producer id], nullptr = unregistered

        // Fallback for source storages with no registered owner (unlinked, uc-14):
        // never committed, so every read against it resolves to invalid.
        detail::ModelStatus never_committed{};
    };

    // ------------------------------------------------------------------
    // Build per-model edge rule tables from the graph nodes. Only FmuModel
    // nodes are registered: pass 1 records output-storage ownership and the
    // per-producer frontier, pass 2 snapshots each model's incoming connections
    // as edges in connections order. `default_mode` is the sampling policy for
    // wired edges; unlinked edges (no registered owner) fall back to Latest.
    // ------------------------------------------------------------------
    DataAccessResolver::DataAccessResolver(std::vector<Invocable *> nodes,
                                           AccessMode default_mode)
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
                e.source = c.source_storage;

                auto owner_it = owner.find(c.source_storage);
                if (owner_it != owner.end())
                {
                    // Wired edge: the graph guarantees the source is one of the
                    // registered producers; sample with the executor's default mode
                    // (StartTime for Jacobi, EndTime for Seidel).
                    e.access.mode = default_mode;
                    e.source_producer = owner_it->second;
                }
                else
                {
                    // Unlinked (uc-14): no registered owner, hence no schedule
                    // happens-before. Stale-only by construction (Latest), which
                    // resolves to invalid while nothing commits to the storage.
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
    // No storage, no I/O, no mutation. Invalid for unknown model/connection or a
    // producer that has not committed yet (D2/D13).
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
        // behaves as never committed (D2/D13 gate).
        const detail::ModelStatus *frontier = &s_->never_committed;
        if (e.source_producer != DataAccessResolver::State::npos &&
            e.source_producer < s_->status.size() &&
            s_->status[e.source_producer] != nullptr)
        {
            frontier = s_->status[e.source_producer].get();
        }

        r = detail::resolve_edge(e.access, *frontier, step_start, step_end);

        // Index mode: the pure core cannot reach the storage; apply the populated
        // gate here (D1).
        if (r.valid && r.is_area && e.access.mode == AccessMode::Index)
        {
            if (e.source == nullptr || e.source->ring == nullptr || !e.source->ring->is_populated(r.area))
            {
                r.valid = false;
            }
        }

        return r;
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