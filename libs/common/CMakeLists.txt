add_library(kb-common STATIC
        src/uuid.cpp
)

target_include_directories(kb-common PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(kb-common
        PRIVATE
        OpenSSL::Crypto
)

target_compile_features(kb-common PUBLIC cxx_std_20)