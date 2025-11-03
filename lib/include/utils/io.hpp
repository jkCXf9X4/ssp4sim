
#pragma once

#include <string>
#include <fstream>

namespace ssp4sim::utils::io
{

    /**
     * @brief Write a string to a file, overwriting any existing content.
     */
    void save_string(const std::string &filename, const std::string &content);
}
