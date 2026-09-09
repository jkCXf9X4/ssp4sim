#include "read_target_core.hpp"

#include <atomic>
#include <cstring>
#include <cstddef>
#include <cstdint>

namespace ssp4sim::scheduling
{
    // ------------------------------------------------------------------
    // detail::edge_from — snapshot the access facts of one connection.
    // `producer` == nullptr ⇒ unlinked read (UC-14): source is not owned by
    // any model in the graph, so there is no schedule happens-before for it
    // and the read must be stale-only.
    //
    // The sampling policy is derived HERE from graph facts, not stored on the
    // ConnectionInfo. Today every edge is sampled at step-start (matching the
    // behaviour the graph builder previously pinned); the delay travels along
    // and is applied as a time-domain shift during resolution. Future policy
    // (feedthrough ⇒ StartTime, delayed ⇒ EndTime, per-schedule phase) is a
    // resolver decision layered on top of these same facts.
    // ------------------------------------------------------------------
    detail::Edge detail::edge_from(const ssp4sim::graph::ConnectionInfo &c,
                                   ssp4sim::graph::Invocable *producer)
    {
        detail::Edge e;
        e.mode = detail::AccessMode::StartTime;
        e.delay = static_cast<std::int64_t>(c.delay);
        e.time_offset = 0;
        e.fixed_index = 0;
        e.unlinked = (producer == nullptr);
        return e;
    }

    // The concrete `detail::ReadResolver` strategies formerly implemented here now live in
    // their own file pairs (read_resolver_*.{hpp,cpp}); this file keeps the storage-free
    // facts and the copy step only.
    bool detail::copy_connection(const ssp4sim::graph::ConnectionInfo &c,
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
            source_area = r.area; // pinned index (Latest / unlinked / Index)
        }
        else if (!c.source_storage->find_latest_valid_area(r.time, source_area))
        {
            return false; // no valid committed data at the resolved time
        }

        std::byte *src = c.source_storage->get_item(source_area, c.source_index);
        std::byte *dst = c.target_storage->get_item(target_area, c.target_index);

        // Type-aware copy (D15). The reader acquire is on the producer's
        // committed_count (see the resolver's resolve), so the value bytes are fully
        // visible before this copy.
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
                std::byte *src_der = c.source_storage->get_derivative(source_area, c.source_index, order);
                std::byte *dst_der = c.target_storage->get_derivative(target_area, c.target_index, order);
                if (src_der != nullptr && dst_der != nullptr)
                {
                    std::memcpy(dst_der, src_der, sizeof(double));
                }
            }
        }

        return true;
    }
}