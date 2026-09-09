#include "read_resolver_latest_executed.hpp"

namespace ssp4sim::scheduling
{
    void detail::LatestExecutedResolver::mark_committed(detail::ModelStatus &status,
                                                        std::uint64_t output_time,
                                                        std::size_t area)
    {
        detail::commit_frontier(status, output_time, area);
    }

    // ------------------------------------------------------------------
    // "Latest executed" resolution. Every connection reads the newest committed area
    // index (zero-order hold), never the live head (determinism). delay/time_offset
    // are time-domain knobs and do not apply. Index-mode edges keep the fixed physical
    // slot (caller applies the populated gate, D1).
    // ------------------------------------------------------------------
    ResolvedRead detail::LatestExecutedResolver::resolve(const detail::Edge &e,
                                                         const detail::ModelStatus &status,
                                                         const ResolverConfig &,
                                                         std::uint64_t,
                                                         std::uint64_t)
    {
        ResolvedRead r{};
        const std::uint64_t generation = status.generation.load();

        // Index mode: absolute physical slot, no time involved (D1).
        if (e.mode == detail::AccessMode::Index)
        {
            r.valid = true;
            r.is_area = true;
            r.area = static_cast<std::size_t>(e.fixed_index);
            r.generation = generation;
            return r;
        }

        // Everything else: newest committed area, direct index, no scan.
        if (status.committed_count.load(std::memory_order::acquire) == 0)
        {
            return r;
        }
        r.valid = true;
        r.is_area = true;
        r.area = status.latest_area.load(std::memory_order::acquire);
        r.generation = generation;
        return r;
    }

    detail::ReadResolver *detail::latest_executed_resolver()
    {
        static detail::LatestExecutedResolver resolver;
        return &resolver;
    }
}