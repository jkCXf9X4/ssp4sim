#include "read_target_core.hpp"

#include <atomic>
#include <cstring>
#include <cstddef>
#include <cstdint>

namespace ssp4sim::scheduling
{


    // Storage-aware copy step of the read path. The pure resolution facts
    // (AccessMode / EdgeAccessRules / ModelStatus / resolve_edge) live in
    // read_resolver.{hpp,cpp} beside the DataAccessResolver; this file keeps only
    // the copy of an already-resolved read (value + derivatives, D15 type-aware).
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