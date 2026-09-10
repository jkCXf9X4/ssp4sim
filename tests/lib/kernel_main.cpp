#include <catch2/catch_session.hpp>

#include "ssp4cpp/utils/log.hpp"

int main(int argc, char* argv[])
{
    // Runtime reality: cone sources create quill loggers without sinks and
    // throw otherwise. Mirror test_main.cpp (no logging seam).
    ssp4cpp::utils::log::add_console(quill::LogLevel::TraceL3);
    return Catch::Session().run(argc, argv);
}