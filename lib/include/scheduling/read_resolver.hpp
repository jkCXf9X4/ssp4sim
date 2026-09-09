#pragma once

#include "read_target_resolver.hpp"
#include "read_target_core.hpp"

#include <cstddef>
#include <cstdint>

// Pluggable read-target resolution policy — the abstract interface for the strategy
// objects that decide "what to read" per connection. Concrete resolvers live in their
// own file pairs:
//   read_resolver_latest_executed.{hpp,cpp}          detail::LatestExecutedResolver
//   read_resolver_macro_step_start_time.{hpp,cpp}    detail::MacroStepStartTimeResolver
//
// The ReadTargetResolver shell (read_target_resolver.hpp) forward-declares `ReadResolver`
// and stores only the pointer; the shell's .cpp and the tests pull this header (or a
// concrete header that includes it) for the full definition.
//
// Dependency direction: the interface depends ON the public resolver header for the value
// types (`ResolvedRead`, `ResolverConfig`) and on `read_target_core.hpp` for the facts it
// operates on (`Edge`, `ModelStatus`). Neither of those drags this file in.
namespace ssp4sim::scheduling
{
    namespace detail
    {
        /// Pluggable read-target resolver. The ReadTargetResolver shell owns one of these
        /// and forwards BOTH the write side (mark_committed) and the read side (resolve)
        /// here, so the access policy no longer lives in the shell or a single switch.
        /// Instances are stateless: the committed frontier lives in `ModelStatus` and the
        /// per-connection facts in `Edge`.
        class ReadResolver
        {
        public:
            virtual ~ReadResolver() noexcept = default;

            /// Advance one producer's committed frontier. Called ONLY after the producer's
            /// output bytes are fully visible (D17 release-store).
            virtual void mark_committed(ModelStatus &status,
                                        std::uint64_t output_time,
                                        std::size_t area) = 0;

            /// Pure resolution: "what to read" for one edge under one producer frontier.
            /// No storage, no I/O, no mutation. Invalid ⇒ producer has not committed yet
            /// (D2/D13 gate).
            virtual ResolvedRead resolve(const Edge &e,
                                         const ModelStatus &status,
                                         const ResolverConfig &cfg,
                                         std::uint64_t step_start,
                                         std::uint64_t step_end) = 0;
        };

        /// Shared frontier-publish step for all resolvers. D17: the caller has already made
        /// the value bytes fully visible; this publishes the new frontier with
        /// release-store ordering.
        void commit_frontier(ModelStatus &status,
                             std::uint64_t output_time,
                             std::size_t area);
    }
}