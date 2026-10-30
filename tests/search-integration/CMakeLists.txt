add_executable(kb-search-integration ManticoreIntegrationTests.cpp)

target_link_libraries(kb-search-integration
        PRIVATE
        kb-search
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(kb-search-integration)