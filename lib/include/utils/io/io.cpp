#include "utils/io/io.hpp"

#include <fstream>
#include <string>
#include <filesystem>

namespace ssp4sim::utils::io
{

    void save_string(const std::string &filename, const std::string &content)
    {
        std::ofstream myfile(filename);
        myfile << content;
    }

    
    void create_parent_folder(const std::string &path)
    {
        namespace fs = std::filesystem;

        fs::path filePath = path;

        // Get the parent directory of the file
        fs::path dir = filePath.parent_path();

        // Create directories if they don't exist
        if (!dir.empty() && !fs::exists(dir)) {
            fs::create_directories(dir);
        }
    }

}

