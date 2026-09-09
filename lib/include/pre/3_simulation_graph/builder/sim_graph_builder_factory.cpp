#include "sim_graph_builder.hpp"

#include "pre/3_simulation/elements/model_fmu.hpp"
#include "utils/fmi/fmu_info.hpp"

#include <memory>

namespace ssp4sim::graph
{

    void GraphBuilder::create_fmu_models(analysis::AnalysisGraphData &graph_data)
    {
        LOG_DEBUG(log, "[{func}] - Create the fmu models", __func__);
        for (auto &model_node : graph_data.model_nodes)
        {
            auto *analysis_model = model_node->source;
            if (!analysis_model)
                continue;

            // Skip models without FMU info
            if (!analysis_model->fmu)
            {
                LOG_DEBUG(log, "[{func}] -- Skipping model without FMU: {model}", __func__, analysis_model->name);
                continue;
            }

            // Create FmuInfo from the SspModel's FMU pointer and transfer ownership to FmuModel
            auto fmu_info = std::make_unique<handler::FmuInfo>(analysis_model->name, analysis_model->fmu.get());

            auto m = std::make_unique<FmuModel>(analysis_model->name, std::move(fmu_info), analysis_model->maxOutputDerivativeOrder);
            LOG_TRACE_L1(log, "[{func}] -- New Model: {model}", __func__, m->name);

            m->delay = analysis_model->delay;
            m->record_inputs = this->record_inputs;
            m->canInterpolateInputs = analysis_model->canInterpolateInputs;
            LOG_DEBUG(log, "[{func}] Model: {model}, delay {delay}", __func__, m->name, m->delay);

            models[analysis_model->name] = std::move(m);
        }
    }

} // namespace ssp4sim::graph