#include <catch2/catch_test_macros.hpp>

#include "simulator.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    namespace fs = std::filesystem;

    std::vector<std::string> splitCsvLine(const std::string& line)
    {
        std::vector<std::string> fields;
        std::string field;
        std::istringstream stream(line);
        while (std::getline(stream, field, ','))
        {
            fields.push_back(field);
        }
        return fields;
    }

    std::string toLower(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
                       [](unsigned char c)
                       {
                           return static_cast<char>(std::tolower(c));
                       });
        return value;
    }

    void trimCpuTimeColumns(const fs::path& csv_path)
    {
        std::ifstream input(csv_path);
        REQUIRE(input.is_open());

        std::string header_line;
        if (!std::getline(input, header_line))
        {
            return;
        }

        const std::vector<std::string> headers = splitCsvLine(header_line);
        std::vector<std::size_t> keep_indices;
        keep_indices.reserve(headers.size());
        for (std::size_t index = 0; index < headers.size(); ++index)
        {
            const std::string lowered = toLower(headers[index]);
            if (lowered.find("cputime") == std::string::npos &&
                lowered.find("walltime") == std::string::npos)
            {
                keep_indices.push_back(index);
            }
        }

        auto join_row = [&keep_indices](const std::vector<std::string>& row)
        {
            std::ostringstream joined;
            bool first = true;
            for (std::size_t index : keep_indices)
            {
                if (index >= row.size())
                {
                    continue;
                }
                if (!first)
                {
                    joined << ',';
                }
                joined << row[index];
                first = false;
            }
            return joined.str();
        };

        std::vector<std::string> output_lines;
        output_lines.reserve(1024);
        output_lines.push_back(join_row(headers));

        std::string line;
        while (std::getline(input, line))
        {
            if (line.empty())
            {
                continue;
            }
            output_lines.push_back(join_row(splitCsvLine(line)));
        }

        std::ofstream output(csv_path, std::ios::trunc);
        REQUIRE(output.is_open());
        for (std::size_t i = 0; i < output_lines.size(); ++i)
        {
            output << output_lines[i];
            if (i + 1 < output_lines.size())
            {
                output << '\n';
            }
        }
    }

    TEST_CASE("embrace scenario generates stable results", "[integration][embrace]")
    {
        const fs::path project_root{SSP4SIM_PROJECT_ROOT};

        const fs::path base_config = project_root / "tests" / "resources" / "embrace_test_parallel.json";
        const fs::path ssp_path = project_root / "resources" / "embrace" / "embrace_scen.ssp";
        REQUIRE(fs::exists(base_config));
        REQUIRE(fs::exists(ssp_path));

        ssp4sim::Simulator simulator(base_config.string());
        simulator.init();
        simulator.simulate();

        std::ifstream config_stream(base_config);
        REQUIRE(config_stream.is_open());
        nlohmann::json config_json;
        config_stream >> config_json;
        const std::string result_file = config_json["simulation"]["recording"]["result_file"].get<std::string>();
        fs::path result_path(result_file);
        if (result_path.is_relative())
        {
            result_path = project_root / result_path;
        }
        trimCpuTimeColumns(result_path);
    }

}
