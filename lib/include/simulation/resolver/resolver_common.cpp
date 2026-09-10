#include "resolver/resolver_common.hpp"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <cstddef>
#include <cstdint>

namespace ssp4sim::scheduling
{

    namespace detail
    {
        // ------------------------------------------------------------------
        // Shared time-domain resolution: see header. `base` is the reference
        // handle of the step-sampled policy; the edge's delay/time_offset shift it.
        // Unlinked / never-committed producers resolve invalid (D2/D13).
        // ------------------------------------------------------------------
        ResolvedRead resolve_time(std::int64_t base,
                                  const EdgeAccessRules &access,
                                  const detail::ModelStatus &status) noexcept
        {
            ResolvedRead r{};

            const std::uint64_t committed_count = status.committed_count.load(std::memory_order::acquire);
            if (committed_count == 0)
            {
                return r; // producer has never committed (D2/D13)
            }

            std::int64_t ref = base + access.time_offset - access.delay;
            // Never read past the committed frontier (M1a); floor at 0 (D8).
            ref = std::min(ref, static_cast<std::int64_t>(status.committed_time.load(std::memory_order::acquire)));
            if (ref < 0)
            {
                ref = 0;
            }

            r.valid = true;
            r.is_area = false;
            r.time = static_cast<std::uint64_t>(ref);
            r.write_counter = committed_count;
            return r;
        }

        // ------------------------------------------------------------------
        // Storage-aware copy step of the read path. The pure resolution facts
        // already reduced the edge to `r`; this performs the type-aware value copy
        // (D15) and derivative forwarding. The reader acquire is on the producer's
        // committed_count (done by the resolver), so the value bytes are fully
        // visible before this copy.
        // ------------------------------------------------------------------
        bool copy_connection(const ssp4sim::graph::ConnectionInfo &c,
                                     std::size_t target_area,
                                     const ResolvedRead &r)
        {
            if (!r.valid)
            {
                return false; // producer not committed yet (D2/D13) — keep init
            }

            std::size_t source_area;
            if (r.is_area)
            {
                source_area = r.area; // pinned index (Latest)
            }
            else if (!c.source_storage->find_latest_valid_area(r.time, source_area))
            {
                return false; // no valid committed data at the resolved time
            }

            std::byte *src = c.source_storage->get_item(source_area, c.source_index);
            std::byte *dst = c.target_storage->get_item(target_area, c.target_index);

            // Type-aware copy (D15).
            if (c.type == ssp4sim::types::DataType::string)
            {
                // Copy-assign to avoid the string-object alias and double-free the old
                // bare-memcpy caused on re-used slots.
                *(std::string *)dst = *(std::string *)src;
            }
            else
            {
                std::memcpy(dst, src, c.size);
            }

            if (c.forward_derivatives)
            {
                for (int order = 1; order <= c.forward_derivatives_order; ++order)
                {
                    if (auto *src = c.source_storage->get_derivative(source_area, c.source_index, order);
                        auto *dst = c.target_storage->get_derivative(target_area, c.target_index, order))
                    {
                        *dst = *src;
                    }
                }

                return true;
            }

            return true; // value copy above succeeded; no derivatives to forward
        }
    }

}