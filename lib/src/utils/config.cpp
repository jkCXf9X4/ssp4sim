#include "utils/config.hpp"

#include "utils/time.hpp"

#include <cctype>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>

namespace ssp4sim::utils
{

    nlohmann::json Config::data_{};

    void Config::loadFromFile(const std::string &path)
    {
        std::ifstream in(path);
        if (!in)
        {
            throw std::runtime_error("Config: cannot open file: " + path);
        }

        std::ostringstream buf;
        buf << in.rdbuf();
        nlohmann::json parsed = nlohmann::json::parse(buf.str(),
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

    const nlohmann::json *Config::resolvePath(const std::string &dottedKey)
    {
        const nlohmann::json *cur = &data_;
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

    std::string Config::substitute_tags(std::string text)
    {
        return std::regex_replace(text, std::regex("\\[TIME\\]"), time::time_now_str());
    }

}

