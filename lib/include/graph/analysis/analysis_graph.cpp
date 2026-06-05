#include "graph/analysis/analysis_graph.hpp"

#include "graph/analysis/components/analysis_model.hpp"
#include "graph/analysis/components/analysis_connector.hpp"
#include "graph/analysis/components/analysis_connection.hpp"

#include "graph/analysis/components/analysis_internal.hpp"

#include "tarjan.hpp"
#include "utils/map.hpp"
#include "utils/node.hpp"

#include <sstream>
#include <utility>

namespace ssp4sim::analysis::graph
{

    AnalysisGraph::AnalysisGraph(std::map<std::string, std::unique_ptr<AnalysisModel>> models_,
                                 std::map<std::string, std::unique_ptr<AnalysisConnector>> connectors_,
                                 std::map<std::string, std::unique_ptr<AnalysisConnection>> connections_,
                                 std::map<std::string, std::unique_ptr<AnalysisModelVariable>> model_variables_)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.graph.AnalysisGraph")),
          models(std::move(models_)),
          connectors(std::move(connectors_)),
          connections(std::move(connections_)),
          model_variables(std::move(model_variables_))
    {
        auto m = ssp4sim::utils::map_ns::map_unique_to_ref(models);
        nodes = ssp4sim::utils::map_ns::map_to_value_vector_copy(m);
    }

    std::vector<AnalysisModel *> AnalysisGraph::get_start_nodes() const
    {
        auto start_nodes = ssp4sim::utils::graph::Node::get_ancestors(nodes);
        return start_nodes;
    }

    std::vector<ssp4sim::utils::graph::Node *> AnalysisGraph::get_nodes() const
    {
        std::vector<ssp4sim::utils::graph::Node *> nodes;
        for (auto &[_, connector] : connectors)
        {
            nodes.push_back(connector.get());
        }
        for (auto &[_, var] : model_variables)
        {
            nodes.push_back(var.get());
        }
        return nodes;
    }


    std::vector<std::vector<utils::graph::Node *>> AnalysisGraph::strongly_connected_components() const
    {
        // Build a node list from connectors and model variables for algebraic loop detection
        auto connector_var_nodes = get_nodes();

        auto strong_system_graph = ssp4sim::utils::graph::strongly_connected_components(
            ssp4sim::utils::graph::Node::cast_to_parent_ptrs(connector_var_nodes));

        return strong_system_graph;
    }

    std::string AnalysisGraph::to_string() const
    {
        auto nodes = get_nodes();
        auto strong_system_graph = strongly_connected_components();

        std::ostringstream oss;
        oss << "Analysis Graph DOT:\n"
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
