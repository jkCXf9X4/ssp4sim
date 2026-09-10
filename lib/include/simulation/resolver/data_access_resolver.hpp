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

    /// Abstract base for data access resolvers. Owns only the shared machinery:
    ///   - per-model vectors of incoming edges, index-aligned with model->connections;
    ///   - per-producer committed frontiers (detail::ModelStatus);
    ///   - mark_committed (D17) and the copy_model_inputs read path.
    /// The resolution policy — "what to read" for one edge — is abstract; each
    /// concrete specialization implements it in resolve_edge().
    /// The shared resolution helpers live in resolver_common.{hpp,cpp}.
    class DataAccessResolver
    {
    public:
        /// Build per-model edge rule tables from the graph nodes. Only FmuModel
        /// nodes are registered (keyed by their Invocable id). Wired edges get
        /// their sampling policy from the concrete subclass; unlinked edges
        /// (source storage without a registered owner) resolve against a
        /// never-committed frontier, so every policy sees them as invalid.
        DataAccessResolver(std::vector<Invocable *> nodes);

        virtual ~DataAccessResolver() noexcept;

        /// Advance one producer's committed frontier. Called ONLY after the producer's
        /// output bytes are fully visible (D17 release-store). No-op for producers the
        /// resolver has not registered.
        void mark_committed(std::size_t model_id,
                            std::uint64_t output_time,
                            std::size_t area);

        /// The entire read path for one model: for each incoming connection, resolve the
        /// read target and copy the source value (and forwarded derivatives) into the
        /// target's input area. Intentionally cheap per connection; the resolver's own
        /// edges are index-aligned with target->connections.
        void copy_model_inputs(ssp4sim::graph::FmuModel *target,
                               std::size_t target_area,
                               std::uint64_t step_start,
                               std::uint64_t step_end);

    protected:
        /// The single specialization point: "what to read" for one edge under one
        /// producer frontier. `access` is the transparent per-edge contract (wire
        /// delay, time_offset, fixed_index, sampling intent); time-domain policies
        /// shift their reference handle by it. No storage, I/O, or mutation.
        /// Unlinked sources arrive via the never-committed fallback frontier and
        /// resolve to invalid (D2/D13).
        virtual ResolvedRead resolve_edge(const EdgeAccessRules &access,
                                          const detail::ModelStatus &status,
                                          std::uint64_t step_start,
                                          std::uint64_t step_end) = 0;

    private:
        // Opaque implementation state (defined in data_access_resolver.cpp). Raw
        // pointer (not unique_ptr) so the header stays complete-type-free.
        struct State;
        State *s_ = nullptr;

        /// Pure resolution: "what to read" for the (model, connection) edge under the
        /// producer frontier, dispatched through the subclass's resolve_edge(). Invalid
        /// for unknown model/connection (D2/D13).
        ResolvedRead resolve(std::size_t model_id,
                             std::size_t connection_id,
                             std::uint64_t step_start,
                             std::uint64_t step_end);
    };
}