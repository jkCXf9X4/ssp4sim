#pragma once

#include "resolver/read_resolver.hpp"

#include "invocable.hpp"
#include "model_connection.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace ssp4sim::scheduling
{

    namespace detail
    {

        bool copy_connection(const ssp4sim::graph::ConnectionInfo &c,
                             std::size_t target_area,
                             const ResolvedRead &r);
    }
}