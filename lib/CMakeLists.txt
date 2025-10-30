
file(GLOB_RECURSE ALL_SRC CONFIGURE_DEPENDS "*.hpp" "*.cpp")

# message(ALL_SRC ssp4sim_lib ="${ALL_SRC}")

add_library(ssp4sim_lib STATIC ${ALL_SRC})

target_include_directories(ssp4sim_lib
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/include/execution
    ${CMAKE_CURRENT_LIST_DIR}/include/graph
    ${CMAKE_CURRENT_LIST_DIR}/include/graph/analysis
    ${CMAKE_CURRENT_LIST_DIR}/include/handler
    ${CMAKE_CURRENT_LIST_DIR}/include/model
    ${CMAKE_CURRENT_LIST_DIR}/include/schema_extensions
    ${CMAKE_CURRENT_LIST_DIR}/include/utils

#  PRIVATE 
#    ${CMAKE_CURRENT_LIST_DIR}/include_private  
)

target_compile_options(ssp4sim_lib PRIVATE -Wfatal-errors)

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(ssp4sim_lib PUBLIC nlohmann_json::nlohmann_json)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.16")
  target_precompile_headers(ssp4sim_lib PRIVATE
    <format>
    <nlohmann/json.hpp>
    <cutecpp/log.hpp>
    <ssp4cpp/fmu.hpp>
    <ssp4cpp/ssp.hpp>
  )
endif()

find_package(TBB CONFIG REQUIRED)
target_link_libraries(ssp4sim_lib PRIVATE TBB::tbb TBB::tbbmalloc)

target_link_libraries(ssp4sim_lib PRIVATE ssp4cpp::ssp4cpp)

target_link_libraries(ssp4sim_lib PRIVATE cutecpp::cutecpp)

target_link_libraries(ssp4sim_lib PUBLIC ssp4sim::3rd::fmi4c)
