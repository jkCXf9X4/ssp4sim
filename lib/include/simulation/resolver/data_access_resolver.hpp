#pragma once

#include "invocable.hpp"
#include "resolver/resolver_common.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ssp4sim::graph
{
    class FmuModel;
}

namespace ssp4sim::scheduling
{
    using ssp4sim::graph::Invocable;

    /// Base resolver. Owns the shared machinery:
    ///   - per-model vectors of incoming edges, index-aligned with model->connections;
    ///   - per-producer committed frontiers (detail::ModelStatus);
    ///   - per-edge AccessMode stamping, mark_committed (D17) and the
    ///     copy_model_inputs read path.
    /// Resolution is per (model, connection): the abstract `resolve()` hook is the
    /// single specialization point, and the protected `edge()` accessor resolves
    /// that edge's transparent contract (rules) together with its producer frontier
    /// (status) in one lookup. The shared per-mode recipes live in
    /// resolver_common.{hpp,cpp}.
    class DataAccessResolver
    {
    public:
        /// Build per-model edge rule tables from the graph nodes. Only FmuModel
        /// nodes are registered (keyed by their Invocable id). Wired edges are
        /// stamped with `default_mode`; unlinked edges (source storage without a
        /// registered owner) are forced to AccessMode::Latest so every policy sees
        /// the stale-only intent (they also resolve invalid: D2/D13).
        DataAccessResolver(std::vector<Invocable *> nodes,
                           AccessMode default_mode = AccessMode::StartTime);

        virtual ~DataAccessResolver() noexcept;

        /// Advance one producer's committed frontier. Called ONLY after the
        /// producer's output bytes are fully visible (D17 release-store). No-op
        /// for producers the resolver has not registered.
        void mark_committed(std::size_t model_id,
                            std::uint64_t output_time,
                            std::size_t area);

        /// The entire read path for one model: for each incoming connection, call
        /// the resolution hook and copy the source value (and forwarded
        /// derivatives) into the target's input area. Intentionally cheap per
        /// connection; the resolver's own edges are index-aligned with
        /// target->connections.
        void copy_model_inputs(ssp4sim::graph::FmuModel *target,
                               std::size_t target_area,
                               std::uint64_t step_start,
                               std::uint64_t step_end);

    protected:
        /// The specialization point: "what to read" for one incoming edge of one
        /// model. The `(model, connection)` pair identifies the edge; its
        /// transparent contract and producer frontier are reachable in one lookup
        /// via edge(). Implementations must consult the producer frontier and
        /// return valid == false while nothing is committed (D2/D13). No storage,
        /// no I/O, no mutation.
        virtual ResolvedRead resolve(std::size_t model_id,
                                     std::size_t connection_id,
                                     std::uint64_t step_start,
                                     std::uint64_t step_end) = 0;

        /// One registered edge's resolution view: the transparent per-edge
        /// contract (wire delay/time_offset + sampling intent) together with the
        /// source producer's committed frontier, in a single bundle.
        struct EdgeAccess
        {
            const EdgeAccessRules *rules = nullptr;      // the edge's transparent contract
            const detail::ModelStatus *status = nullptr; // the producer's committed frontier
        };

        /// Single-lookup accessor backing resolve(): returns the resolution view
        /// for one (model, connection). Unknown inputs yield neutral Latest rules
        /// against a never-committed frontier (D2/D13).
        EdgeAccess edge(std::size_t model_id, std::size_t connection_id);

        /// Override one registered edge's sampling mode (graph-structure resolvers
        /// use this after the uniform default). No-op for unknown model/connection.
        void stamp_edge_mode(std::size_t model_id,
                             std::size_t connection_id,
                             AccessMode mode);

    private:
        // Opaque implementation state (defined in data_access_resolver.cpp). Raw
        // pointer (not unique_ptr) so the header stays complete-type-free.
        struct State;
        State *s_ = nullptr;
    };
}