#pragma once

#include "read_resolver.hpp"

#include <cstddef>
#include <cstdint>

// "Macro step start time" read policy — a concrete detail::ReadResolver strategy, split
// into its own file pair so each policy is independently unit-testable and readable.
namespace ssp4sim::scheduling
{
    namespace detail
    {
        /// "Macro step start time" policy — the graph default. Wired edges sample at the
        /// macro step start handle (shifted by delay/time_offset, clamped to the committed
        /// frontier M1a, floor 0 D8); unlinked (UC-14) / Latest edges read the newest
        /// committed area; Index edges keep the fixed slot.
        class MacroStepStartTimeResolver : public ReadResolver
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
        /// frontier lives in `ModelStatus`. This is the default used when the shell is
        /// constructed with a nullptr resolver.
        ReadResolver *macro_step_start_time_resolver();
    }
}