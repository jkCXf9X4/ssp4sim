#pragma once

#include "utils/time.hpp"

#include <nlohmann/json_fwd.hpp>
#include <string>

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
        static void loadFromString(const std::string &config);

        static void data_available();

        static std::string as_string();

        static std::string getString(const std::string &dottedKey);
        static double getDouble(const std::string &dottedKey);
        static int getInt(const std::string &dottedKey);
        static bool getBool(const std::string &dottedKey);

        static std::string getOr(const std::string &dottedKey, const std::string &defaultValue);
        static double getOr(const std::string &dottedKey, double defaultValue);
        static int getOr(const std::string &dottedKey, int defaultValue);
        static bool getOr(const std::string &dottedKey, bool defaultValue);

        static std::string substitute_tags(std::string text);
    };

}
