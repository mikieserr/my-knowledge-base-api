find_package(miniocpp REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLER_CPP REQUIRED IMPORTED_TARGET poppler-cpp)
find_package(libzip CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)

add_library(kb-attachments STATIC
        src/text_extractor.cpp
        src/s3_file_storage.cpp
        src/attachment_service.cpp
)

target_include_directories(kb-attachments PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(kb-attachments
        PUBLIC
        kb-common
        kb-search
        miniocpp::miniocpp
        PRIVATE
        OpenSSL::Crypto
        ZLIB::ZLIB
        Threads::Threads
        PkgConfig::POPPLER_CPP
        libzip::zip
        pugixml::pugixml
)

target_compile_features(kb-attachments PUBLIC cxx_std_20)