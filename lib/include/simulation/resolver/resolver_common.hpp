#pragma once

#include "model_connection.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace ssp4sim::scheduling
{
    /// Result of resolving one edge for the current step: exactly one of
    /// `area` / `time` is meaningful, selected by `is_area`:
    ///   - is_area: `area` is the physical storage slot to read (no search);
    ///   - else:    `time` is a viable, frontier-clamped reference time (search ≤ it).
    /// `valid == false` means the producer has not committed yet (D2/D13) — the
    /// consumer keeps its initialization / skips this frame.
    struct ResolvedRead
    {
        bool valid = false;
        bool is_area = false;
        std::size_t area = 0;
        std::uint64_t time = 0;
        std::uint64_t write_counter = 0;
    };

    /// Sampling policy for one edge. Historically per-connection; today the
    /// concrete resolver owns the policy (it *is* one of these), so `mode` stays
    /// as the transparent statement of sampling intent while the resolver class
    /// remains the authority. ConnectionInfo carries only wire facts; the resolver
    /// derives policy from those plus the schedule.
    enum class AccessMode : int
    {
        StartTime, // sample at step_start
        EndTime,   // sample at step_end
        Latest,    // newest committed index (zero-order hold), no time lookup
        Index      // fixed physical slot (fixed_index)
    };

    /// Pure per-edge access facts — the transparent contract handed to every
    /// resolve_edge() policy. Derived from a ConnectionInfo + graph context.
    struct EdgeAccessRules
    {
        // Neutral default (newest committed index). Unlinked edges (source storage
        // without a registered owner) are stamped Latest by the resolver's
        // constructor: stale-only by construction (uc-14).
        AccessMode mode = AccessMode::Latest;
        std::int64_t delay = 0;       // connection wire delay (time-domain policies shift by it)
        std::int64_t time_offset = 0; // extra sampling shift applied on top of delay
        std::int64_t fixed_index = 0; // Index-mode physical slot; unused by the current policies
    };

    namespace detail
    {
        /// Committed frontier of one producer — the single mutable truth (M1b).
        /// Written only by mark_committed with release-store ordering (D17); read
        /// with acquire ordering in the resolution helpers. `committed_count` is
        /// the gate: it is incremented LAST so a reader that acquires on it sees
        /// the whole frontier.
        struct ModelStatus
        {
            std::atomic<std::uint64_t> committed_count = 0;
            std::atomic<std::uint64_t> committed_time = 0;
            std::atomic<std::uint64_t> latest_area = 0;
        };

        /// Shared time-domain resolution for every step-sampled policy. `base` is
        /// the reference handle the policy samples at (step_start or step_end),
        /// shifted by the edge's delay/time_offset, clamped to the producer's
        /// committed frontier (M1a), floored at 0 (D8) and gated on the producer
        /// having committed (D2/D13). No storage, no I/O, no mutation.
        ResolvedRead resolve_time(std::int64_t base,
                                  const EdgeAccessRules &access,
                                  const detail::ModelStatus &status) noexcept;

        /// Storage-aware copy step of the read path. The pure resolution facts
        /// (ModelStatus / resolve_time / concrete resolver policies) live beside
        /// the DataAccessResolver; this keeps only the copy of an already-resolved
        /// read (value + derivatives, D15 type-aware).
        bool copy_connection(const ssp4sim::graph::ConnectionInfo &c,
                             std::size_t target_area,
                             const ResolvedRead &r);
    }
}