#include "read_target_resolver.hpp"

#include "read_target_core.hpp"
#include "read_resolver_latest_executed.hpp"
#include "read_resolver_macro_step_start_time.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <utility>

namespace ssp4sim::scheduling
{
    // ------------------------------------------------------------------
    // ReadTargetResolver — all private tables behind the opaque State pointer.
    // The chosen resolution policy (detail::edge_from facts + a detail::ReadResolver
    // strategy) is implemented in read_target_core.cpp and shared with tests directly.
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
        detail::ReadResolver *resolver = nullptr;
        ResolverConfig cfg;
    };

    ReadTargetResolver::ReadTargetResolver(std::vector<ssp4sim::graph::Invocable *> models,
                                           detail::ReadResolver *resolver,
                                           ResolverConfig cfg)
        : s_(new ReadTargetResolver::State)
    {
        s_->cfg = cfg;
        s_->resolver = (resolver != nullptr) ? resolver : detail::macro_step_start_time_resolver();

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

        // Forwarded to the injected resolver (D17 release-store ordering guaranteed by the
        // caller; the resolver publishes the new frontier).
        s_->resolver->mark_committed(*it->second, output_time, area);
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

        r = s_->resolver->resolve(re.access, *status, s_->cfg, step_start, step_end);

        // Index mode: apply the populated gate (the pure core cannot reach the storage).
        if (r.valid && r.is_area && re.access.mode == detail::AccessMode::Index)
        {
            if (re.source == nullptr || re.source->ring == nullptr ||
                !re.source->ring->is_populated(r.area))
            {
                r.valid = false;
            }
        }

        return r;
    }

    void ReadTargetResolver::copy_model_inputs(ssp4sim::graph::FmuModel *target,
                                               std::size_t target_area,
                                               std::uint64_t step_start,
                                               std::uint64_t step_end)
    {
        const std::vector<ssp4sim::graph::ConnectionInfo> &connections = target->connections;
        for (std::size_t i = 0; i < connections.size(); ++i)
        {
            // Intentionally cheap per connection; the resolver's own edges are
            // index-aligned with target->connections.
            const ResolvedRead r = resolve(target, i, step_start, step_end);
            detail::copy_connection(connections[i], target_area, r);
        }
    }
}