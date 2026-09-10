#include "read_resolver.hpp"

#include "read_target_core.hpp"

#include "pre/3_simulation_graph/elements/model_fmu.hpp"
#include "signal/storage.hpp"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ssp4sim::scheduling
{
    TODO: 
    Populate the class
    use vector and index access, Invocable has a id that is unique 0..N
    connections are also static after setup, index from vector can be used as asses 
    something model[model_id].connection[connection_id] to get the edge rules
    
    This should be quite simple and fast
    
    common constructor should set up the vectors and access patterns 
    
    specialized/derived classes should enable specialization regarding access rules


    DataAccessResolver::DataAccessResolver(std::vector<Invocable *> nodes)
    {

    }


    void DataAccessResolver::copy_model_inputs(ssp4sim::graph::FmuModel *target,
                                               std::size_t target_area,
                                               std::uint64_t step_start,
                                               std::uint64_t step_end)
    {
        if (s_ == nullptr || target == nullptr)
        {
            return;
        }

        const std::vector<ssp4sim::graph::ConnectionInfo> &connections = target->connections;

        for (std::size_t i = 0; i < connections.size(); ++i)
        {
            // Intentionally cheap per connection; the resolver's own edges are
            // index-aligned with target->connections.
            const ResolvedRead r = resolve(target->id, i, step_start, step_end);
            detail::copy_connection(connections[i], target_area, r);
        }
    }

}