add_executable(kb-common-tests
        test_uuid.cpp
)

target_link_libraries(kb-common-tests
        PRIVATE
        kb-common
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(kb-common-tests)