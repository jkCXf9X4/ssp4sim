#pragma once

#include "invocable.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace ssp4sim::graph
{

    /**
     * @brief An Invocable that wraps the sub-nodes of a strongly connected
     *        component.
     *
     * Acts as the single representative node for an SCC inside a condensed
     * graph: it is itself an Invocable, so it can be placed in a new graph
     * and its children/parents can be rewired to form the SCC DAG. Its
     * default invoke runs the wrapped sub-nodes sequentially once per step;
     * schedulers that need loop relaxation (e.g. Jacobi sub-stepping) use
     * the sub-nodes directly instead.
     */
    class SccGroup final : public Invocable
    {
    public:
        // The sub-nodes contained in this strongly connected component.
        std::vector<Invocable *> sub_nodes;

        explicit SccGroup(std::vector<Invocable *> sub_nodes);

        // true when the component is a loop: more than one node, or a
        // single node with a self-edge (feedback self-reference).
        bool is_loop() const;

        std::string to_string() const override;

        uint64_t invoke(StepData step_data) override final;
    };

}