#pragma once

#include "read_resolver.hpp"

#include <cstddef>
#include <cstdint>

// "Latest executed" read policy — a concrete detail::ReadResolver strategy, split into
// its own file pair so each policy is independently unit-testable and readable.
namespace ssp4sim::scheduling
{
    namespace detail
    {
        /// "Latest executed" policy: every connection reads the producer's newest committed
        /// area (zero-order hold), never the live head (D2/D7). Index-mode edges keep their
        /// fixed physical slot (the caller applies the populated gate).
        class LatestExecutedResolver : public ReadResolver
        {
        public:
            void mark_committed(ModelStatus &status,
                                std::uint64_t output_time,
                                std::size_t area) override;
            ResolvedRead resolve(const Edge &e,
                                 const ModelStatus &status,
                                 const ResolverConfig &cfg,
                                 std::uint64_t step_start,
                                 std::uint64_t step_end) override;
        };

        /// Shared instance. Safe to share: resolvers are stateless, all per-producer
        /// frontier lives in `ModelStatus`.
        ReadResolver *latest_executed_resolver();
    }
}