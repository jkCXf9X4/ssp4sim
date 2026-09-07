#include "read_target_core.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace ssp4sim::scheduling
{
    // ------------------------------------------------------------------
    // detail::edge_from — snapshot the access facts of one connection.
    // `producer` == nullptr ⇒ unlinked read (UC-14): source is not owned by
    // any model in the graph, so there is no schedule happens-before for it
    // and the read must be stale-only.
    // ------------------------------------------------------------------
    detail::Edge detail::edge_from(const ssp4sim::graph::ConnectionInfo &c,
                                   ssp4sim::graph::Invocable *producer)
    {
        detail::Edge e;
        e.mode = c.mode;
        e.delay = static_cast<std::int64_t>(c.delay);
        e.time_offset = c.time_offset;
        e.fixed_index = c.fixed_index;
        e.unlinked = (producer == nullptr);
        return e;
    }

    // ------------------------------------------------------------------
    // detail::resolve_edge — the pure resolution core.
    //   - Latest / unlinked: the latest committed area index (no time involved; never
    //                        the live head — determinism).
    //   - StartTime/EndTime: step_start / step_end shifted by delay/offset, clamped to
    //                        the producer's committed frontier (M1a), floor 0 (D8).
    //   - Index:             the fixed physical slot; caller applies the populated gate.
    //   - invalid:           producer has not committed yet (D2/D13 gate).
    // No input_time is passed: `Latest` reads the newest committed index, so a time
    // lookup is unnecessary.
    // ------------------------------------------------------------------
    ResolvedRead detail::resolve_edge(const detail::Edge &e,
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
        if (e.unlinked || e.mode == ssp4sim::graph::DataAccessMode::Latest)
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
        if (e.mode == ssp4sim::graph::DataAccessMode::Index)
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
            case ssp4sim::graph::DataAccessMode::StartTime:
                base = step_start;
                break;
            case ssp4sim::graph::DataAccessMode::EndTime:
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
}