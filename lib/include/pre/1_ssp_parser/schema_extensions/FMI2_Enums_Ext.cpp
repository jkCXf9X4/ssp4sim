
#include "ssp4cpp/utils/log.hpp"

#include "FMI2_Enums_Ext.hpp"

#include <string>
#include <stdexcept>

namespace ssp4sim::ext::fmi2
{

    namespace enums
    {
        ssp4cpp::utils::log::Logger *log()
        {
            // Cache this logger locally so we avoid eager header initialization.
            static ssp4cpp::utils::log::Logger *logger =
                ssp4cpp::utils::log::make_logger("ssp4sim.ext.fmi2.enums");
            return logger;
        }

        /**
         * @brief  Return the in-memory size (in bytes) of a single value
         *         represented by the given DataType.
         */
        std::size_t get_data_type_size(types::DataType t)
        {
            switch (t)
            {
            case types::DataType::boolean:
            case types::DataType::integer:
            case types::DataType::enumeration:
                return sizeof(int); // typically 4
            case types::DataType::real:
                return sizeof(double); // typically 8
            case types::DataType::string:
                return sizeof(std::string);
            case types::DataType::unknown:
                return 0;
            }
            // If the enum gains a new value and the switch isn’t updated,
            // this keeps the compiler happy in -Wall/-Wswitch-enums builds.
            throw std::invalid_argument("Unknown DataType");
        }

        std::string data_type_to_string(types::DataType type, void *data)
        {
            IF_LOG({
                LOG_TRACE_L2(log(), "[{func}] init", __func__);
            });
            switch (type)
            {
            case types::DataType::real:
                return std::to_string(*(double *)data);
            case types::DataType::boolean:
                return std::to_string(*(bool *)data);
            case types::DataType::integer:
            case types::DataType::enumeration:
                return std::to_string(*(int *)data);
            case types::DataType::string:
                return *(std::string *)data;
            default:
                return "<bin>";
            }
        }

        DefaultValue get_default_value(types::DataType type)
        {
            switch (type)
            {
            case types::DataType::boolean:
                return false;

            case types::DataType::integer:
            case types::DataType::enumeration:
                return 0;

            case types::DataType::real:
                return 0.0;

            case types::DataType::string:
                return "";

            case types::DataType::unknown:
                return 0;

            default:
                throw std::runtime_error("Unknown type");
            }
        }
    }

}
