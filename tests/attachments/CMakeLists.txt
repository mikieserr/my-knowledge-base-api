add_executable(kb-attachments-tests
        AttachmentTests.cpp
)

target_include_directories(kb-attachments-tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)

target_link_libraries(kb-attachments-tests
        PRIVATE
        kb-attachments
        kb-common
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(kb-attachments-tests)