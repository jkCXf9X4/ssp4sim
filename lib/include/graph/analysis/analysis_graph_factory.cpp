#include "analysis/analysis_graph_factory.hpp"

#include "analysis/components/analysis_connector.hpp"
#include "analysis/components/analysis_connection.hpp"
#include "analysis/components/analysis_model.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <unordered_map>
#include <set>
#include <unordered_set>

namespace ssp4sim::analysis
{
    namespace
    {
        ssp4cpp::utils::log::Logger *log()
        {
            static ssp4cpp::utils::log::Logger *logger =
                ssp4cpp::utils::log::make_logger("ssp4sim.analysis.AnalysisGraphFactory");
            return logger;
        }
    }

    AnalysisGraphFactory(AnalysisSystem *analysis_system_)
    {
        analysis_system = analysis_system_;

        auto analysis_system_tree = build_tree(analysis_system.get());

        LOG_DEBUG(log(), "[{func}] Analysis tree:\n", __func__, analysis_system_tree->tree_string());
    }



    void AnalysisSystemBuilder::build_full_graph(AnalysisGraphData *data)
    {
        for (auto &connections : data.connection_nodes)
        {
            
        }
    }

    void AnalysisSystemBuilder::build_model_graph(AnalysisGraphData *data)
    {
        for (auto &connections : data.connection_nodes)
        {
            
        }
    }



    std::vector<std::unique_ptr<ModelNode>> AnalysisSystemBuilder::build_model_graph()
    {
        // Implement the most simple solution
        // Store nodes in this->model_graph
    }

} // namespace ssp4sim::analysis