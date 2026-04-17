#include <catch2/catch_session.hpp>

#include "ssp4cpp/utils/log.hpp"


int main(int argc, char* argv[])
{
    // add default console sink for tests
    ssp4cpp::utils::log::add_console(quill::LogLevel::TraceL3);
    return Catch::Session().run(argc, argv);
}
