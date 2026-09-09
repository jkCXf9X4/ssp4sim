#pragma once

#include <cstddef>
#include <cstdint>

namespace ssp4sim::scheduling
{
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
        // Neutral default (newest committed index). edge_from() derives the actual
        // policy from graph facts (currently StartTime for every wired edge, matching
        // the sampling the graph builder previously pinned).
        AccessMode mode = AccessMode::Latest;
        std::int64_t delay = 0;
        std::int64_t time_offset = 0;
        std::int64_t fixed_index = 0;
    };

    class DataAccessResolver
    {
    public:
        DataAccessResolver() = default;
        ~DataAccessResolver() noexcept = default;

        /// Advance one producer's committed frontier. Called ONLY after the producer's
        /// output bytes are fully visible (D17 release-store).
        void mark_committed(std::size_t model_id,
                            std::uint64_t output_time,
                            std::size_t area);

        /// Pure resolution: "what to read" for one edge under one producer frontier.
        /// No storage, no I/O, no mutation.
        ResolvedRead resolve(std::size_t model_id,
                             std::size_t connection_id,
                             std::uint64_t step_start,
                             std::uint64_t step_end);
    };
}