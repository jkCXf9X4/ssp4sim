#pragma once

#include "_ssp_item.hpp"
#include "ssp_connector.hpp"

#include <cstdint>
#include <string>

namespace ssp4sim::analysis
{

    class SspConnection : public SspItem
    {
    public:
        std::string source_model;
        std::string source_connector;
        std::string target_model;
        std::string target_connector;

        uint64_t delay = 0;

        bool is_boundary = false;

        
        SspConnection(std::string source_model_,
                           std::string source_connector_,
                           std::string target_model_,
                           std::string target_connector_);

        void set_custom(uint64_t delay_ = 0)
        {
            delay = delay_;
        }

        std::string to_string() const;
    };

} // namespace ssp4sim::analysis