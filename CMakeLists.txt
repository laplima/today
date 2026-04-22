cmake_minimum_required(VERSION 3.30)

project(today)

set(sources
	Activity.cpp
	main.cpp
)

find_package(spdlog REQUIRED)
find_package(colibry REQUIRED)

# MOVED TO PRESETS
# set(CMAKE_CXX_STANDARD 26)
# set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")

# INCLUDES
include_directories("/opt/local/include")

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE ${sources})
target_link_libraries(${PROJECT_NAME} spdlog::spdlog_header_only colibry::colibry)
target_compile_options(${PROJECT_NAME} PUBLIC)
