#include "graph/analysis/analysis_graph.hpp"

#include "graph/analysis/analysis_model.hpp"
#include "graph/analysis/analysis_connector.hpp"
#include "graph/analysis/analysis_connection.hpp"

#include "tarjan.hpp"
#include "utils/map.hpp"

#include <sstream>
#include <utility>

namespace ssp4sim::analysis::graph
{

    AnalysisGraph::AnalysisGraph(std::map<std::string, std::unique_ptr<AnalysisModel>> models_,
                                 std::map<std::string, std::unique_ptr<AnalysisConnector>> connectors_,
                                 std::map<std::string, std::unique_ptr<AnalysisConnection>> connections_)
        : models(std::move(models_)),
          connectors(std::move(connectors_)),
          connections(std::move(connections_))
    {
        auto m = ssp4sim::utils::map_ns::map_unique_to_ref(models);
        nodes = ssp4sim::utils::map_ns::map_to_value_vector_copy(m);
    }

    std::vector<AnalysisModel *> AnalysisGraph::get_start_nodes() const
    {
        auto start_nodes = ssp4sim::utils::graph::Node::get_ancestors(nodes);
        return start_nodes;
    }

    std::string AnalysisGraph::to_string() const
    {
        auto strong_system_graph = ssp4sim::utils::graph::strongly_connected_components(ssp4sim::utils::graph::Node::cast_to_parent_ptrs(nodes));

        std::ostringstream oss;
        oss << "Simulation Graph DOT:\n"
            << ssp4sim::utils::graph::Node::to_dot(nodes) << "\n"
            << ssp4sim::utils::graph::ssc_to_string(strong_system_graph)
            << "\nStart nodes:\n";

        for (auto &model : get_start_nodes())
        {
            oss << "Model: " << model->to_string() << "\n";
        }
        return oss.str();
    }

}
