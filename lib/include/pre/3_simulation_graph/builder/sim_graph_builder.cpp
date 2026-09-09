#include "sim_graph_builder.hpp"

#include "pre/3_simulation/elements/model_fmu.hpp"
#include "signal/recorder.hpp"

#include <memory>
#include <utility>

namespace ssp4sim::graph
{

    GraphBuilder::GraphBuilder(bool record_inputs)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.graph.GraphBuilder")),
          record_inputs(record_inputs)
    {
    }

    FmuModel* GraphBuilder::as_fmu(Invocable* invocable) {
        return dynamic_cast<FmuModel*>(invocable);
    }

    std::map<std::string, std::unique_ptr<Invocable>> GraphBuilder::build(analysis::AnalysisGraphData *graph_data)
    {
        LOG_DEBUG(log, "[{func}] init with pre-resolved graph data", __func__);

        create_fmu_models(*graph_data);
        create_data_storage_areas(*graph_data);
        wire_connections(*graph_data);
        derive_model_edges(*graph_data);

        LOG_DEBUG(log, "[{func}] - Allocate the input/output areas", __func__);
        for (auto &[ssp_resource_name, model] : models)
        {
            auto m = as_fmu(model.get());
            if (!m)
            {
                LOG_WARNING(log, "[{func}] Skipping model '{name}' with null FmuModel pointer", __func__, ssp_resource_name);
                continue;
            }
            m->input_area->allocate();
            m->output_area->allocate();
        }

        LOG_DEBUG(log, "[{func}] exit", __func__);
        return std::move(models);
    }



} // namespace ssp4sim::graph