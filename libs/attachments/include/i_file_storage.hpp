#pragma once
#include "attachment.hpp"
#include <istream>
#include <memory>
#include <string>

namespace kb::attachments {

    struct StoredObject {
        std::string key;
        std::uint64_t size{0};
        std::string checksum_sha256;
    };

    class IFileStorage {
    public:
        virtual ~IFileStorage() = default;

        virtual StoredObject put(const std::string& original_name,
                                 std::istream& data,
                                 std::uint64_t expected_size) = 0;

        virtual std::unique_ptr<std::istream> get(const std::string& key) = 0;
        virtual void remove(const std::string& key) = 0;
        virtual bool exists(const std::string& key) = 0;
    };

}