#pragma once

#include "executor/seidel/seidel_base.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace ssp4sim::graph
{
    class ParallelSeidel final : public SeidelBase
    {
    public:
        ParallelSeidel(std::vector<std::shared_ptr<Invocable>> nodes);

        std::string to_string() const override
        {
            return "ParallelSeidel:\n{}\n";
        }

        /**
         * Parallel Seidel traversal is not implemented yet; invoke() always
         * throws std::runtime_error("This is not implemented").
         */
        uint64_t invoke(StepData step_data) override final;
    };
}