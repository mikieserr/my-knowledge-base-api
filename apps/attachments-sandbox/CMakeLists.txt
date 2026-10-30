add_executable(attachments-sandbox
        src/main.cpp
)

target_link_libraries(attachments-sandbox
        PRIVATE
        kb-attachments
        kb-common
)