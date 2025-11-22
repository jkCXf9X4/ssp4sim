#include "utils/config.hpp"

#include "utils/time.hpp"

#include <nlohmann/json.hpp>

#include <cctype>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace ssp4sim::utils
{

    nlohmann::json Config::data_{};

    namespace
    {
        // Resolve "a.b.c" into a json node pointer (or nullptr if missing).
        nlohmann::json *resolvePathNode(nlohmann::json &data, const std::string &dottedKey)
        {
            nlohmann::json *cur = &data;
            if (dottedKey.empty())
            {
                return cur;
            }

            size_t pos = 0;
            while (cur && pos != std::string::npos)
            {
                size_t dot = dottedKey.find('.', pos);
                std::string key = (dot == std::string::npos)
                                      ? dottedKey.substr(pos)
                                      : dottedKey.substr(pos, dot - pos);

                if (!key.empty() && std::all_of(key.begin(), key.end(), [](unsigned char ch)
                                                { return std::isdigit(ch) != 0; }))
                {
                    size_t idx = static_cast<size_t>(std::stoul(key));
                    if (!cur->is_array() || idx >= cur->size())
                    {
                        return nullptr;
                    }
                    cur = &(*cur)[idx];
                }
                else
                {
                    if (!cur->is_object() || !cur->contains(key))
                    {
                        return nullptr;
                    }
                    cur = &(*cur)[key];
                }

                if (dot == std::string::npos)
                {
                    break;
                }
                pos = dot + 1;
            }
            return cur;
        }

        template <typename T>
        T read_json_value(const nlohmann::json &node)
        {
            return node.get<T>();
        }

        template <>
        std::string read_json_value<std::string>(const nlohmann::json &node)
        {
            return Config::substitute_tags(node.get<std::string>());
        }

        template <typename T>
        T get_value(const std::string &dottedKey)
        {
            Config::data_available();

            const nlohmann::json *node = resolvePathNode(Config::data_, dottedKey);
            if (!node || node->is_null())
            {
                throw missing_key_error("Config: missing key: " + dottedKey);
            }

            try
            {
                return read_json_value<T>(*node);
            }
            catch (const std::exception &e)
            {
                throw type_error("Config: type error for key '" + dottedKey +
                                 "': " + std::string(e.what()));
            }
        }
    }

    nlohmann::json *Config::resolvePath(const std::string &dottedKey)
    {
        data_available();
        return resolvePathNode(data_, dottedKey);
    }

    void Config::loadFromFile(const std::string &path)
    {
        std::ifstream in(path);
        if (!in)
        {
            throw std::runtime_error("Config: cannot open file: " + path);
        }

        std::ostringstream buf;
        buf << in.rdbuf();
        loadFromString(buf.str());
    }

    void Config::loadFromString(const std::string &config)
    {
        nlohmann::json parsed = nlohmann::json::parse(config,
                                                      /* callback */ nullptr,
                                                      /* allow exceptions */ true,
                                                      /* ignore_comments */ true);

        data_ = std::move(parsed);
    }

    void Config::data_available()
    {
        if (data_.is_null())
        {
            throw std::runtime_error("Config: data is not loaded");
        }
    }

    std::string Config::as_string()
    {
        data_available();
        return data_.dump();
    }

    std::string Config::substitute_tags(std::string text)
    {
        return std::regex_replace(text, std::regex("\\[TIME\\]"), time::time_now_str());
    }

    std::string Config::getString(const std::string &dottedKey)
    {
        return get_value<std::string>(dottedKey);
    }

    double Config::getDouble(const std::string &dottedKey)
    {
        return get_value<double>(dottedKey);
    }

    int Config::getInt(const std::string &dottedKey)
    {
        return get_value<int>(dottedKey);
    }

    bool Config::getBool(const std::string &dottedKey)
    {
        return get_value<bool>(dottedKey);
    }

    std::string Config::getOr(const std::string &dottedKey, const std::string &defaultValue)
    {
        try
        {
            return getString(dottedKey);
        }
        catch (const missing_key_error &)
        {
            return defaultValue;
        }
    }

    double Config::getOr(const std::string &dottedKey, double defaultValue)
    {
        try
        {
            return getDouble(dottedKey);
        }
        catch (const missing_key_error &)
        {
            return defaultValue;
        }
    }

    int Config::getOr(const std::string &dottedKey, int defaultValue)
    {
        try
        {
            return getInt(dottedKey);
        }
        catch (const missing_key_error &)
        {
            return defaultValue;
        }
    }

    bool Config::getOr(const std::string &dottedKey, bool defaultValue)
    {
        try
        {
            return getBool(dottedKey);
        }
        catch (const missing_key_error &)
        {
            return defaultValue;
        }
    }

}
