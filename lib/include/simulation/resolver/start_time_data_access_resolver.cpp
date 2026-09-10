#include "resolver/start_time_data_access_resolver.hpp"

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace ssp4sim::scheduling
{
    StartTimeDataAccessResolver::StartTimeDataAccessResolver(std::vector<Invocable *> nodes)
        : DataAccessResolver(std::move(nodes))
    {
    }

    // ------------------------------------------------------------------
    // StartTime — sample the producer at step_start (perfectly parallel /
    // Jacobi): every model reads the inputs produced in the previous step.
    // ------------------------------------------------------------------
    ResolvedRead StartTimeDataAccessResolver::resolve_edge(const EdgeAccessRules &access,
                                                           const detail::ModelStatus &status,
                                                           std::uint64_t step_start,
                                                           std::uint64_t)
    {
        return detail::resolve_time(static_cast<std::int64_t>(step_start), access, status);
    }
}