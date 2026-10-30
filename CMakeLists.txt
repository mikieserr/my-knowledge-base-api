cmake_minimum_required(VERSION 3.20)
project(knowledge-base LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/dependencies.cmake)
include(cmake/testing.cmake)

enable_testing()

add_subdirectory(libs/common)
add_subdirectory(libs/search)
add_subdirectory(libs/attachments)
add_subdirectory(apps/attachments-sandbox)
add_subdirectory(tests/attachments)
add_subdirectory(tests/attachments-integration)
add_subdirectory(tests/search-integration)
add_subdirectory(tests/common)


