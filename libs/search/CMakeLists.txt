find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

add_library(kb-search STATIC src/manticore_search_index.cpp)
target_include_directories(kb-search PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(kb-search PRIVATE CURL::libcurl nlohmann_json::nlohmann_json)