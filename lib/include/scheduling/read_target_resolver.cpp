#include "read_target_resolver.hpp"

#include "read_target_core.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <utility>

namespace ssp4sim::scheduling
{
    // ------------------------------------------------------------------
    // ReadTargetResolver — all private tables behind the opaque State pointer.
    // The pure resolution core (detail::edge_from / detail::resolve_edge) is
    // implemented in read_target_core.cpp and shared with tests directly.
    // ------------------------------------------------------------------

    // ------------------------------------------------------------------
    // The opaque implementation state. This completes the incomplete
    // `ReadTargetResolver::State` declared in the header; it is the single
    // private table holder of the class.
    // ------------------------------------------------------------------
    struct ReadTargetResolver::State
    {
        struct RegisteredEdge
        {
            detail::Edge access;
            ssp4sim::graph::Invocable *source_producer = nullptr;
            ssp4sim::signal::SignalStorage *source = nullptr;
        };

        std::unordered_map<ssp4sim::graph::Invocable *, std::vector<RegisteredEdge>> edges;
        std::unordered_map<ssp4sim::graph::Invocable *, std::unique_ptr<detail::ModelStatus>> status;
        std::unordered_map<ssp4sim::signal::SignalStorage *, ssp4sim::graph::Invocable *> owner;
        ResolverConfig cfg;
    };

    ReadTargetResolver::ReadTargetResolver(std::vector<ssp4sim::graph::Invocable *> models,
                                           ResolverConfig cfg)
        : s_(new ReadTargetResolver::State)
    {
        s_->cfg = cfg;

        // Pass 1: register ownership + centralized status per FmuModel.
        for (const auto &m : models)
        {
            auto *fmu = dynamic_cast<ssp4sim::graph::FmuModel *>(m);
            if (fmu == nullptr)
            {
                continue;
            }
            s_->owner[fmu->output_area.get()] = m;
            s_->status.emplace(m, std::make_unique<detail::ModelStatus>());
        }

        // Pass 2: snapshot each model's incoming connections as edges,
        // index-aligned with model->connections.
        for (const auto &m : models)
        {
            auto *fmu = dynamic_cast<ssp4sim::graph::FmuModel *>(m);
            if (fmu == nullptr)
            {
                continue;
            }

            std::vector<ReadTargetResolver::State::RegisteredEdge> edges;
            edges.reserve(fmu->connections.size());

            for (const ssp4sim::graph::ConnectionInfo &c : fmu->connections)
            {
                auto owner_it = s_->owner.find(c.source_storage);
                ssp4sim::graph::Invocable *producer =
                    (owner_it != s_->owner.end()) ? owner_it->second : nullptr;

                edges.push_back(ReadTargetResolver::State::RegisteredEdge{
                    detail::edge_from(c, producer),
                    producer,
                    c.source_storage});
            }

            if (!edges.empty())
            {
                s_->edges.emplace(m, std::move(edges));
            }
        }
    }

    ReadTargetResolver::~ReadTargetResolver()
    {
        delete s_;
        s_ = nullptr;
    }

    void ReadTargetResolver::mark_committed(ssp4sim::graph::Invocable *producer,
                                            std::uint64_t output_time,
                                            std::size_t area)
    {
        auto it = s_->status.find(producer);
        if (it == s_->status.end())
        {
            return; // unregistered producer — nothing to advance
        }

        detail::ModelStatus &st = *it->second;

        // D17: the caller has already made the value bytes fully visible;
        // publish the new frontier with release-store ordering.
        st.committed_count.fetch_add(1, std::memory_order::release);
        st.committed_time.store(output_time, std::memory_order::release);
        st.latest_area.store(area, std::memory_order::release);
        st.generation.fetch_add(1, std::memory_order::release);
    }

    ResolvedRead ReadTargetResolver::resolve(ssp4sim::graph::Invocable *model,
                                             std::size_t connection_idx,
                                             std::uint64_t step_start,
                                             std::uint64_t step_end)
    {
        ResolvedRead r{};

        auto edges_it = s_->edges.find(model);
        if (edges_it == s_->edges.end() || connection_idx >= edges_it->second.size())
        {
            return r; // unknown model / connection
        }

        const ReadTargetResolver::State::RegisteredEdge &re = edges_it->second[connection_idx];

        // Status of the source producer; an unregistered producer behaves as never committed.
        detail::ModelStatus fallback_status{};
        const detail::ModelStatus *status = &fallback_status;
        auto status_it = s_->status.find(re.source_producer);
        if (status_it != s_->status.end())
        {
            status = status_it->second.get();
        }

        r = detail::resolve_edge(re.access, *status, s_->cfg, step_start, step_end);

        // Index mode: apply the populated gate (the pure core cannot reach the storage).
        if (r.valid && r.is_area && re.access.mode == ssp4sim::graph::DataAccessMode::Index)
        {
            if (re.source == nullptr || re.source->ring == nullptr ||
                !re.source->ring->is_populated(r.area))
            {
                r.valid = false;
            }
        }

        return r;
    }
}