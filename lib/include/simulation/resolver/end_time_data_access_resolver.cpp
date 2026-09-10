#include "resolver/end_time_data_access_resolver.hpp"

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace ssp4sim::scheduling
{
    EndTimeDataAccessResolver::EndTimeDataAccessResolver(std::vector<Invocable *> nodes)
        : DataAccessResolver(std::move(nodes))
    {
    }

    // ------------------------------------------------------------------
    // EndTime — sample the producer at step_end (Seidel friendly): a consumer may
    // use data produced earlier in the same sweep.
    // ------------------------------------------------------------------
    ResolvedRead EndTimeDataAccessResolver::resolve_edge(const EdgeAccessRules &access,
                                                         const detail::ModelStatus &status,
                                                         std::uint64_t,
                                                         std::uint64_t step_end)
    {
        return detail::resolve_time(static_cast<std::int64_t>(step_end), access, status);
    }
}