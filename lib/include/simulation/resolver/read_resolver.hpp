#pragma once

#include "invocable.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

// TODO: rename file to reflect class name

namespace ssp4sim::graph
{
    class FmuModel;
}

namespace ssp4sim::scheduling
{
    using ssp4sim::graph::Invocable;

    struct ResolvedRead
    {
        bool valid = false;
        bool is_area = false;
        std::size_t area = 0;
        std::uint64_t time = 0;
        std::uint64_t write_counter = 0;
    };

    /// Sampling policy for one edge — owned entirely by the resolver. ConnectionInfo
    /// carries only wire facts; the resolver derives policy from those plus schedule.
    enum class AccessMode : int
    {
        StartTime, // sample at step_start
        EndTime,   // sample at step_end
        Latest,    // newest committed index (zero-order hold), no time lookup
        Index      // fixed physical slot (fixed_index)
    };

    /// Pure per-connection access facts; derived from a ConnectionInfo + graph context.
    struct EdgeAccessRules
    {
        // Neutral default (newest committed index). The resolver's constructor derives
        // the actual policy from graph facts: `default_mode` for wired edges (currently
        // StartTime, matching the sampling the graph builder previously pinned) and
        // Latest for unlinked edges (source storage without a registered owner).
        AccessMode mode = AccessMode::Latest;
        std::int64_t delay = 0;
        std::int64_t time_offset = 0;
        std::int64_t fixed_index = 0;
    };

    namespace detail
    {
        /// Committed frontier of one producer — the single mutable truth (M1b).
        /// Written only by mark_committed with release-store ordering (D17); read
        /// with acquire ordering in resolve_edge. `committed_count` is the gate:
        /// it is incremented LAST so a reader that acquires on it sees the whole
        /// frontier.
        struct ModelStatus
        {
            std::atomic<std::uint64_t> committed_count = 0;
            std::atomic<std::uint64_t> committed_time = 0;
            std::atomic<std::uint64_t> latest_area = 0;
        };

        /// Pure per-edge resolution: "what to read" for one edge under one producer
        /// frontier. No storage, no I/O, no mutation. `Latest`/`Index` select a
        /// physical area; `StartTime`/`EndTime` a viable, frontier-clamped reference
        /// time. Invalid while the producer has not committed anything (D2/D13) and
        /// while the source is unlinked (uc-14: no registered owner commits to it).
        ResolvedRead resolve_edge(const EdgeAccessRules &access,
                                  const detail::ModelStatus &status,
                                  std::uint64_t step_start,
                                  std::uint64_t step_end);
    }


    class DataAccessResolver
    {
    public:
        DataAccessResolver() = default;

        /// Build per-model edge rule tables from the graph nodes. Only
        /// FmuModel nodes are registered (keyed by their Invocable id).
        /// `default_mode` is the sampling policy for wired edges; unlinked
        /// edges (source storage without a registered owner) fall back to
        /// AccessMode::Latest (stale-only).
        DataAccessResolver(std::vector<Invocable *> nodes,
                           AccessMode default_mode = AccessMode::StartTime);

        ~DataAccessResolver() noexcept;

        /// Advance one producer's committed frontier. Called ONLY after the producer's
        /// output bytes are fully visible (D17 release-store). No-op for producers the
        /// resolver has not registered.
        void mark_committed(std::size_t model_id,
                            std::uint64_t output_time,
                            std::size_t area);

        /// Pure resolution: "what to read" for one edge under one producer frontier.
        /// No storage, no I/O, no mutation. Invalid for unknown model/connection or a
        /// producer that has not committed yet (D2/D13).

        ResolvedRead resolve(std::size_t model_id,
                             std::size_t connection_id,
                             std::uint64_t step_start,
                             std::uint64_t step_end);

        /// The entire read path for one model: for each incoming connection, resolve the
        /// read target and copy the source value (and forwarded derivatives) into the
        /// target's input area. Intentionally cheap per connection; the resolver's own
        /// edges are index-aligned with target->connections.
        void copy_model_inputs(ssp4sim::graph::FmuModel *target,
                               std::size_t target_area,
                               std::uint64_t step_start,
                               std::uint64_t step_end);

    private:
        // Opaque implementation state (defined in read_resolver.cpp). Raw
        // pointer (not unique_ptr) so the header stays complete-type-free.
        struct State;
        State *s_ = nullptr;
    };
}