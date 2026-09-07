#pragma once

#include "read_target_resolver.hpp"

#include "invocable.hpp"
#include "model_connection.hpp"

#include <cstddef>
#include <cstdint>

// Pure, storage-free access-resolution core of the read-target resolver.
// Lives in its own file pair so it is independently unit-testable.
//
// Dependency direction: the core depends ON the public resolver header for its
// value types (`ResolvedRead`, `ResolverConfig`) and `ConnectionInfo`/`Invocable`;
// the resolver header never drags the core in. The resolver's .cpp links both.
//
// Determinism contract (design docs): single committed-watermark truth (M1b),
// reads clamped to the committed frontier (M1a/D2/D7), unlinked reads stale-only
// (UC-14). The frontier is written (release-store) only through the resolver's
// mark_committed.
namespace ssp4sim::scheduling
{
    namespace detail
    {
        /// Pure per-connection access facts; a snapshot of the relevant ConnectionInfo
        /// fields, taken from a connection at construction time.
        struct Edge
        {
            ssp4sim::graph::DataAccessMode mode = ssp4sim::graph::DataAccessMode::Latest;
            std::int64_t delay = 0;
            std::int64_t time_offset = 0;
            std::int64_t fixed_index = 0;
            bool unlinked = false;                 // UC-14: no owning producer in the graph
        };

        /// Centralized per-producer committed frontier. The only mutable runtime state (M1b).
        struct ModelStatus
        {
            std::atomic<std::uint64_t> committed_count{0};
            std::atomic<std::uint64_t> committed_time{0};
            std::atomic<std::uint64_t> latest_area{0};
            std::atomic<std::uint64_t> generation{0};
        };

        /// Map one connection's runtime-access facts into an Edge. `producer` is the owner
        /// of `c.source_storage`, or nullptr for unlinked reads (source not owned by any
        /// model in the graph — UC-14).
        Edge edge_from(const ssp4sim::graph::ConnectionInfo &c,
                       ssp4sim::graph::Invocable *producer);

        /// Pure resolution core: "what is the latest / appropriate / valid read target" for
        /// one edge under one producer frontier. No storage, no I/O, no mutation.
        ///   - Latest / unlinked: the latest committed area index (no time involved).
        ///   - StartTime/EndTime:  step_start / step_end, shifted by delay/offset and
        ///                         clamped to the committed frontier.
        ///   - Index:              the fixed physical slot (caller applies the populated gate).
        ///   - invalid:            producer has not committed yet (D2/D13 gate).
        /// No `input_time` is needed: Latest reads the newest committed index directly.
        ResolvedRead resolve_edge(const Edge &e,
                                  const ModelStatus &status,
                                  const ResolverConfig &cfg,
                                  std::uint64_t step_start,
                                  std::uint64_t step_end);
    }
}