#include "resolver/latest_data_access_resolver.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace ssp4sim::scheduling
{
    LatestDataAccessResolver::LatestDataAccessResolver(std::vector<Invocable *> nodes)
        : DataAccessResolver(std::move(nodes))
    {
    }

    // ------------------------------------------------------------------
    // Latest — zero-order hold on the newest committed area. No time involved.
    // Unlinked sources resolve against the never-committed fallback frontier and
    // are invalid (D2/D13-safe, keep init).
    // ------------------------------------------------------------------
    ResolvedRead LatestDataAccessResolver::resolve_edge(const EdgeAccessRules &,
                                                        const detail::ModelStatus &status,
                                                        std::uint64_t,
                                                        std::uint64_t)
    {
        ResolvedRead r{};

        const std::uint64_t committed_count = status.committed_count.load(std::memory_order::acquire);
        if (committed_count == 0)
        {
            return r; // never committed / unlinked (D2/D13)
        }

        r.valid = true;
        r.is_area = true;
        r.area = status.latest_area.load(std::memory_order::acquire);
        r.write_counter = committed_count;
        return r;
    }
}