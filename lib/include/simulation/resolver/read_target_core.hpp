#pragma once

#include "read_target_resolver.hpp"

#include "invocable.hpp"
#include "model_connection.hpp"

#include <cstddef>
#include <cstdint>

// Pure, storage-free access-facts core of the read-target resolver: the per-connection
// edge facts (`AccessMode`, `Edge`), the centralized committed frontier (`ModelStatus`),
// and the storage-free helpers (`edge_from`, `copy_connection`). Lives in its own file
// pair so it is independently unit-testable.
//
// The pluggable read policy lives in its own file pair: `read_resolver.hpp` defines the
// `detail::ReadResolver` interface and `read_resolver_{latest_executed,macro_step_start_time}.{hpp,cpp}`
// the concrete strategies. The ReadTargetResolver shell (read_target_resolver.hpp/.cpp)
// injects one and forwards its `mark_committed` / `resolve`.
//
// Dependency direction: this core depends ON the public resolver header for its
// value types (`ResolvedRead`, `ResolverConfig`) and `ConnectionInfo`/`Invocable`;
// the resolver header never drags the core in. The resolver's .cpp links both.
//
// Determinism contract (design docs): single committed-watermark truth (M1b),
// reads clamped to the committed frontier (M1a/D2/D7), unlinked reads stale-only
// (UC-14). The frontier is written (release-store) only through the resolver's
// mark_committed.
namespace ssp4sim::scheduling
{

        /// Execute one already-resolved read: copy the source value (and forwarded
        /// derivatives) into the connection's target storage area. Type-aware (D15):
        /// strings are copy-assigned, everything else is memcpy'd. The resolved read's
        /// `valid`/`is_area`/`area`/`time` are interpreted per payload.
        /// Returns false if the resolved read was invalid or no committed source area
        /// matched (target untouched).
        bool copy_connection(const ssp4sim::graph::ConnectionInfo &c,
                             std::size_t target_area,
                             const ResolvedRead &r);
}