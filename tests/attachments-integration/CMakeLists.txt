add_executable(kb-attachments-integration
        S3IntegrationTests.cpp
)


target_link_libraries(kb-attachments-integration
        PRIVATE
        kb-attachments
        kb-common
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(kb-attachments-integration)