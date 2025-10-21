
#pragma once

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>
#include <string>

namespace ssp4sim::utils::json
{

    using Json = nlohmann::json;

    inline Json parse_json(const std::string &s)
    {
        return Json::parse(s);
    }

    inline std::string to_string(const Json &j)
    {
        return j.dump();
    }

    inline Json parse_json_file(const std::string &filename)
    {
        std::ifstream in(filename);
        if (!in)
        {
            throw std::runtime_error("Could not open file: " + filename);
        }
        return Json::parse(in);
    }

}
