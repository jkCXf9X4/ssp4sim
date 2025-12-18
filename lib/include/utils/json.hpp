
#pragma once

#include <nlohmann/json.hpp>

#include <string>

namespace ssp4sim::utils::json
{

    using Json = nlohmann::json;

    Json parse_json(const std::string &s);

    std::string to_string(const Json &j);

    Json parse_json_file(const std::string &filename);

}
