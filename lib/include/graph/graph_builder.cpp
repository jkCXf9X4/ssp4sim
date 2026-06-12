#include "graph/graph_builder.hpp"

#include "analysis/components/analysis_model.hpp"
#include "analysis/components/analysis_connector.hpp"
#include "analysis/components/analysis_connection.hpp"
#include "model/model_fmu.hpp"
#include "utils/map.hpp"

#include <cstdint>
#include <memory>
#include <set>
#include <utility>
#include <fstream>

namespace ssp4sim::graph
{
    GraphBuilder::GraphBuilder(const analysis::AnalysisSystem &analysis_system_,
                               ssp4sim::signal::DataRecorder *recorder,
                               ssp4sim::SharedConfig *config)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.graph.GraphBuilder")),
          analysis_system(analysis_system_),
          recorder(recorder),
          config(config)
    {
    }

    void GraphBuilder::build()
    {
        LOG_DEBUG(log, "[{func}] init", __func__);

        create_fmu_models();
        create_data_storage_areas();
        wire_connections();
        derive_model_edges();

        LOG_DEBUG(log, "[{func}] - Allocate the input/output areas", __func__);
        for (auto &[ssp_resource_name, model] : models)
        {
            auto m = dynamic_cast<FmuModel *>(model.get());
            if (!m)
            {
                LOG_WARNING(log, "[{func}] Skipping model '{name}' with null FmuModel pointer", __func__, ssp_resource_name);
                continue;
            }
            m->input_area->allocate();
            m->output_area->allocate();
            if (recorder)
            {
                if (m->record_inputs)
                {
                    recorder->add_storage(m->input_area.get());
                }
                recorder->add_storage(m->output_area.get());
            }
        }

        LOG_DEBUG(log, "[{func}] exit", __func__);
    }

    void GraphBuilder::create_fmu_models()
    {
        LOG_DEBUG(log, "[{func}] - Create the fmu models", __func__);
        for (auto *analysis_model : analysis_system.get_all_models())
        {
            // Skip models without FMU info (e.g., system containers)
            if (!analysis_model->fmu)
            {
                LOG_DEBUG(log, "[{func}] -- Skipping model without FMU: {model}", __func__, analysis_model->name);
                continue;
            }

            auto m = std::make_unique<FmuModel>(analysis_model->name, analysis_model->fmu, analysis_model->maxOutputDerivativeOrder);
            LOG_TRACE_L1(log, "[{func}] -- New Model: {model}", __func__, m->name);

            m->delay = analysis_model->delay;
            m->record_inputs = this->config->record_inputs;
            LOG_DEBUG(log, "[{func}] Model: {model}, delay {delay}", __func__, m->name, m->delay);

            models[analysis_model->name] = std::move(m);
        }
    }

    void GraphBuilder::create_data_storage_areas()
    {
        LOG_DEBUG(log, "[{func}] - Create the data storage areas within the model", __func__);
        auto start_value_log_file = std::ofstream(this->config->start_value_log_file, std::ios::out);
        for (auto *analysis_model : analysis_system.get_all_models())
        {
            if (!analysis_model->fmu)
                continue;

            auto model = dynamic_cast<FmuModel *>(models[analysis_model->name].get());
            if (!model)
            {
                LOG_WARNING(log, "[{func}] Model {name} not found in model map, skipping", __func__, analysis_model->name);
                continue;
            }

            for (auto &connector : analysis_model->connectors)
            {
                ConnectorInfo info;
                info.type = connector->data_type;
                info.size = connector->size;
                info.name = connector->name;

                info.forward_derivatives = connector->forward_derivatives;
                info.forward_derivatives_order = connector->forward_derivatives_order;

                info.value_ref = connector->value_reference;

                info.fmu = model->fmu;

                if (connector->initial_value)
                {
                    info.initial_value = std::make_unique<ext::ssp1::ssv::StartValue>(*connector->initial_value);

                    auto value = ext::fmi2::enums::data_type_to_string(info.type, info.initial_value->raw_ptr());

                    LOG_TRACE_L1(log, "[{func}] -- Store start value for {} : {}", __func__, info.name, value);
                    start_value_log_file << connector->causality << ", " << connector->name << ", " << value << "\n";
                }

                if (connector->causality == types::Causality::input)
                {
                    info.index = static_cast<uint32_t>(model->input_area->add(connector->name, connector->data_type, connector->forward_derivatives_order));
                    info.storage = model->input_area.get();
                    model->inputs[connector->name] = std::move(info);
                }
                else if (connector->causality == types::Causality::output)
                {
                    info.index = static_cast<uint32_t>(model->output_area->add(connector->name, connector->data_type, connector->forward_derivatives_order));
                    info.storage = model->output_area.get();
                    model->outputs[connector->name] = std::move(info);
                }
                else if (connector->causality == types::Causality::parameter)
                {
                    info.index = static_cast<uint32_t>(-1);
                    model->parameters[connector->name] = std::move(info);
                }
            }
        }
    }

    // Resolve a bare model name to a key in the models map (exact then suffix match)
    std::string GraphBuilder::resolve_model_key(const std::string &name) const
    {
        if (name.empty()) return "";
        if (models.contains(name)) return name;
        std::string dot_name = "." + name;
        for (auto &[key, _] : models)
        {
            if (key.size() > dot_name.size() &&
                key.substr(key.size() - dot_name.size()) == dot_name)
            {
                return key;
            }
        }
        return "";
    }

    // Resolve a connection through the system hierarchy.
    // For connections where source or target is a system name, chase through
    // boundary connectors inside that system to find the actual FMU model.
    std::vector<GraphBuilder::ResolvedConnection> GraphBuilder::resolve_connection(
        const analysis::AnalysisConnection *conn) const
    {
        std::vector<ResolvedConnection> result;
        std::string src_key = resolve_model_key(conn->source_model);
        std::string tgt_key = resolve_model_key(conn->target_model);

        // Case 1: Both are valid FMU model keys — use directly
        if (!src_key.empty() && !tgt_key.empty())
        {
            result.push_back({src_key, conn->source_connector,
                              tgt_key, conn->target_connector,
                              conn->delay});
            return result;
        }

        // Case 2: Source is an FMU model, target is a system name
        // Resolve by finding the system and walking through its boundary
        if (!src_key.empty() && tgt_key.empty() && !conn->target_model.empty())
        {
            auto *nested = analysis_system.get_nested_system(conn->target_model);
            if (nested)
            {
                for (auto &inner : nested->connections)
                {
                    if (!inner->is_boundary_crossing) continue;
                    if (inner->source_model != "") continue;
                    if (inner->source_connector != conn->target_connector) continue;
                    // The inner target model is a bare name — prefix with system name
                    std::string inner_tgt = nested->name + "." + inner->target_model;
                    inner_tgt = resolve_model_key(inner_tgt);
                    if (inner_tgt.empty()) continue;
                    result.push_back({src_key, conn->source_connector,
                                      inner_tgt, inner->target_connector,
                                      std::max(conn->delay, inner->delay)});
                }
            }
            return result;
        }

        // Case 3: Target is an FMU model, source is a system name
        if (!tgt_key.empty() && src_key.empty() && !conn->source_model.empty())
        {
            auto *nested = analysis_system.get_nested_system(conn->source_model);
            if (nested)
            {
                for (auto &inner : nested->connections)
                {
                    if (!inner->is_boundary_crossing) continue;
                    if (inner->source_model != "") continue;
                    if (inner->source_connector != conn->source_connector) continue;
                    std::string inner_tgt = nested->name + "." + inner->target_model;
                    inner_tgt = resolve_model_key(inner_tgt);
                    if (inner_tgt.empty()) continue;
                    result.push_back({inner_tgt, inner->target_connector,
                                      tgt_key, conn->target_connector,
                                      std::max(conn->delay, inner->delay)});
                }
                // Also check for boundary OUTPUT connections (FMU feeds the boundary)
                for (auto &inner : nested->connections)
                {
                    if (!inner->is_boundary_crossing) continue;
                    if (inner->target_model != "") continue;   // Not a boundary OUTPUT
                    if (inner->target_connector != conn->source_connector) continue;
                    std::string inner_src = nested->name + "." + inner->source_model;
                    inner_src = resolve_model_key(inner_src);
                    if (inner_src.empty()) continue;
                    result.push_back({inner_src, inner->source_connector,
                                      tgt_key, conn->target_connector,
                                      std::max(conn->delay, inner->delay)});
                }
            }
            return result;
        }

        // Case 4: Boundary-crossing connection (one end is empty)
        // Try to resolve using suffix matching for the non-empty side
        if (conn->is_boundary_crossing)
        {
            if (!conn->target_model.empty())
            {
                std::string resolved = resolve_model_key(conn->target_model);
                if (!resolved.empty())
                {
                    result.push_back({conn->source_model, conn->source_connector,
                                      resolved, conn->target_connector,
                                      conn->delay});
                }
            }
        }

        return result;
    }

    void GraphBuilder::wire_connections()
    {
        LOG_DEBUG(log, "[{func}] - Hand the information regarding the connections over to the model", __func__);

        // Collect all connections, both original and resolved
        std::vector<ResolvedConnection> resolved_conns;
        for (auto *connection : analysis_system.get_all_connections())
        {
            auto resolved = resolve_connection(connection);
            if (resolved.empty())
            {
                LOG_DEBUG(log, "[{func}] Skipping unresolvable connection {src}.{sc} -> {tgt}.{tc}",
                          __func__, connection->source_model, connection->source_connector,
                          connection->target_model, connection->target_connector);
            }
            resolved_conns.insert(resolved_conns.end(),
                                  std::make_move_iterator(resolved.begin()),
                                  std::make_move_iterator(resolved.end()));
        }

        for (auto &conn : resolved_conns)
        {
            auto src_it = models.find(conn.source_model);
            auto tgt_it = models.find(conn.target_model);

            if (src_it == models.end() || tgt_it == models.end())
            {
                LOG_WARNING(log, "[{func}] Skipping resolved connection {src}.{sc} -> {tgt}.{tc}: model not found",
                            __func__, conn.source_model, conn.source_connector,
                            conn.target_model, conn.target_connector);
                continue;
            }

            auto source_model = dynamic_cast<FmuModel *>(src_it->second.get());
            auto target_model = dynamic_cast<FmuModel *>(tgt_it->second.get());

            auto source_connector_name = conn.source_model + "." + conn.source_connector;
            auto target_connector_name = conn.target_model + "." + conn.target_connector;

            auto &source_conn = source_model->outputs[source_connector_name];
            auto &target_conn = target_model->inputs[target_connector_name];

            ConnectionInfo con_info;
            con_info.type = source_conn.type;
            con_info.size = source_conn.size;

            con_info.source_storage = source_model->output_area.get();
            con_info.target_storage = target_model->input_area.get();
            con_info.source_index = source_conn.index;
            con_info.target_index = target_conn.index;

            con_info.forward_derivatives = source_conn.forward_derivatives;
            con_info.forward_derivatives_order = source_conn.forward_derivatives_order;

            con_info.delay = conn.delay;

            // Resolve feedthrough from the analysis connector
            auto *src_analysis_conn = analysis_system.find_connector(
                conn.source_model,
                source_connector_name);
            con_info.is_feedthrough = src_analysis_conn ? src_analysis_conn->is_feedthrough : false;
            if (conn.delay > 0)
            {
                con_info.is_feedthrough = false;
            }

            LOG_TRACE_L1(log, "[{func}] Connection: {src}.{sc} -> {tgt}.{tc}, delay {delay}",
                         __func__,
                         conn.source_model, conn.source_connector,
                         conn.target_model, conn.target_connector,
                         conn.delay);

            target_model->connections.push_back(std::move(con_info));
        }
    }

    void GraphBuilder::derive_model_edges()
    {
        LOG_DEBUG(log, "[{func}] Deriving model-to-model edges from connection graph", __func__);
        std::set<std::pair<std::string, std::string>> model_pairs;

        // Resolve all connections (same as wire_connections)
        for (auto *connection : analysis_system.get_all_connections())
        {
            auto resolved = resolve_connection(connection);
            for (auto &r : resolved)
            {
                model_pairs.insert({r.source_model, r.target_model});
            }
        }

        LOG_DEBUG(log, "[{func}] Found {count} unique model pairs", __func__, model_pairs.size());
        for (auto &[source_name, target_name] : model_pairs)
        {
            LOG_TRACE_L1(log, "[{func}] - Model edge: {source} -> {target}", __func__, source_name, target_name);
            if (!models.contains(source_name))
            {
                LOG_WARNING(log, "[{func}] Source model {name} not found in model map, skipping", __func__, source_name);
                continue;
            }
            if (!models.contains(target_name))
            {
                LOG_WARNING(log, "[{func}] Target model {name} not found in model map, skipping", __func__, target_name);
                continue;
            }
            models[source_name]->add_child(models[target_name].get());
        }
    }

    std::unique_ptr<Graph> GraphBuilder::get_graph()
    {
        return std::make_unique<Graph>(ssp4sim::utils::map_ns::map_unique_to_ref(models), recorder);
    }

    std::map<std::string, std::unique_ptr<Invocable>> GraphBuilder::get_models()
    {
        return std::move(models);
    }

}