
#include "SSP_Ext.hpp"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ssp4sim::ext::ssp
{
    namespace
    {
        ssp4cpp::utils::log::Logger *log()
        {
            // Cache this logger locally so we avoid eager header initialization.
            static ssp4cpp::utils::log::Logger *logger =
                ssp4cpp::utils::log::make_logger("ssp4sim.ext.ssp");
            return logger;
        }
    }



}
