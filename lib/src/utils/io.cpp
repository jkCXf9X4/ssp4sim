#include "utils/io.hpp"

#include <fstream>
#include <string>

namespace ssp4sim::utils::io
{

    void save_string(const std::string &filename, const std::string &content)
    {
        std::ofstream myfile(filename);
        myfile << content;
    }

}

