#pragma once

#include "utils/time.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <optional>
#include <shared_mutex>
#include <type_traits>
#include <ranges>
#include <regex>

namespace ssp4sim::utils
{

    struct config_error : std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };
    struct missing_key_error : config_error
    {
        using config_error::config_error;
    };
    struct type_error : config_error
    {
        using config_error::config_error;
    };

    class Config
    {
    public:
        static nlohmann::json data_;

        // Load (or reload) JSON config from a file path.
        // Throws std::runtime_error on I/O or parse errors.
        static void loadFromFile(const std::string &path);

        static void data_available();

        static std::string as_string();

        // Get a required value; throws if missing or type mismatch.
        template <class T>
        static T get(const std::string &dottedKey)
        {
            data_available();

            const nlohmann::json *node = resolvePath(dottedKey);
            if (!node || node->is_null())
            {
                throw missing_key_error("Config: missing key: " + dottedKey);
            }

            try
            {
                if constexpr (std::is_same_v<T, std::string>)
                {
                    return substitute_tags(node->get<T>());
                }
                else
                {
                    return node->get<T>();
                }
            }
            catch (const std::exception &e)
            {
                throw type_error("Config: type error for key '" + dottedKey +
                                 "': " + std::string(e.what()));
            }
        }

        // Get with a default; returns defaultValue when key is missing.
        // Throws on type mismatch (to avoid silently bad configs).
        template <class T>
        static T getOr(const std::string &dottedKey, T defaultValue)
        {
            data_available();

            try
            {
                return get<T>(dottedKey);
            }
            catch (const missing_key_error &e)
            {
                return defaultValue;
            }
        }

        // Resolve "a.b.c" into a json node pointer (or nullptr if missing).
        static const nlohmann::json *resolvePath(const std::string &dottedKey);

        static std::string substitute_tags(std::string text);
    };

}
