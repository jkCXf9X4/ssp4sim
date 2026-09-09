#include "read_resolver_macro_step_start_time.hpp"

namespace ssp4sim::scheduling
{
    void detail::MacroStepStartTimeResolver::mark_committed(detail::ModelStatus &status,
                                                            std::uint64_t output_time,
                                                            std::size_t area)
    {
        detail::commit_frontier(status, output_time, area);
    }

    // ------------------------------------------------------------------
    // "Macro step start time" resolution — the graph default (the layer of edge_from).
    //   - StartTime/EndTime: step_start / step_end shifted by delay/offset, clamped to
    //                        the producer's committed frontier (M1a), floor 0 (D8).
    //   - Latest / unlinked: newest committed area (same as LatestExecuted).
    //   - Index:             the fixed physical slot; caller applies the populated gate.
    //   - invalid:           producer has not committed yet (D2/D13 gate).
    // ------------------------------------------------------------------
    ResolvedRead detail::MacroStepStartTimeResolver::resolve(const detail::Edge &e,
                                                             const detail::ModelStatus &status,
                                                             const ResolverConfig &cfg,
                                                             std::uint64_t step_start,
                                                             std::uint64_t step_end)
    {
        ResolvedRead r{};

        const std::uint64_t committed_count = status.committed_count.load(std::memory_order::acquire);
        const std::uint64_t generation = status.generation.load();

        // Latest (default) / unlinked: newest committed area, direct index, no scan.
        // delay/time_offset are time-domain knobs and do not apply to the latest index.
        if (e.unlinked || e.mode == detail::AccessMode::Latest)
        {
            if (committed_count == 0)
            {
                return r;
            }
            r.valid = true;
            r.is_area = true;
            r.area = status.latest_area.load(std::memory_order::acquire);
            r.generation = generation;
            return r;
        }

        // Index mode: absolute physical slot, no time involved. The caller is
        // responsible for checking the slot is populated / otherwise valid (D1).
        if (e.mode == detail::AccessMode::Index)
        {
            r.valid = true;
            r.is_area = true;
            r.area = static_cast<std::size_t>(e.fixed_index);
            r.generation = generation;
            return r;
        }

        // StartTime / EndTime: pick the step handle the connection samples at.
        std::uint64_t base;
        switch (e.mode)
        {
            case detail::AccessMode::StartTime:
                base = step_start;
                break;
            case detail::AccessMode::EndTime:
            default:
                base = step_end;
                break;
        }

        // Not-yet-committed producer (first-commit / t=0, D2/D13) → no valid data.
        if (committed_count == 0)
        {
            return r;
        }

        std::int64_t ref = static_cast<std::int64_t>(base) + e.time_offset - e.delay;
        if (cfg.clamp_stale_shortfall)
        {
            ref = std::min(ref, static_cast<std::int64_t>(status.committed_time.load(std::memory_order::acquire)));
        }
        if (ref < 0)
        {
            ref = 0;
        }

        r.valid = true;
        r.is_area = false;
        r.time = static_cast<std::uint64_t>(ref);
        r.generation = generation;
        return r;
    }

    detail::ReadResolver *detail::macro_step_start_time_resolver()
    {
        static detail::MacroStepStartTimeResolver resolver;
        return &resolver;
    }
}