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
    /// single specialization point, and the protected helpers expose that edge's
    /// transparent contract (edge_rules) and its producer frontier
    /// (producer_status) so a concrete policy can implement its decision directly.
    /// The shared per-mode recipes live in resolver_common.{hpp,cpp}.
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
        /// transparent contract and producer frontier are reachable through
        /// edge_rules() / producer_status(). Implementations must consult the
        /// producer frontier and return valid == false while nothing is committed
        /// (D2/D13). No storage, no I/O, no mutation.
        virtual ResolvedRead resolve(std::size_t model_id,
                                     std::size_t connection_id,
                                     std::uint64_t step_start,
                                     std::uint64_t step_end) = 0;

        /// Transparent per-connection contract for the edge (model_id,
        /// connection_id): wire delay/time_offset + sampling intent. Unknown
        /// model/connection returns a neutral Latest stub.
        const EdgeAccessRules &edge_rules(std::size_t model_id,
                                          std::size_t connection_id);

        /// Committed frontier of the edge's source producer. Unlinked edges /
        /// unknown producers resolve against a never-committed frontier (D2/D13).
        const detail::ModelStatus &producer_status(std::size_t model_id,
                                                   std::size_t connection_id);

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