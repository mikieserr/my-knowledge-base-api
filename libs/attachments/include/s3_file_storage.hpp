#pragma once

#include "i_file_storage.hpp"
#include "errors.hpp"

#include <miniocpp/client.h>
#include <miniocpp/credentials.h>
#include <miniocpp/providers.h>

#include <memory>
#include <string>

namespace kb::attachments {

    struct S3Config {
        std::string endpoint;      // http://localhost:9000
        std::string access_key;
        std::string secret_key;
        std::string bucket;
        bool        use_ssl{false};
    };

    class S3FileStorage : public IFileStorage {
    public:
        explicit S3FileStorage(S3Config cfg);

        StoredObject put(const std::string& original_name,
                         std::istream& data,
                         std::uint64_t expected_size) override;

        std::unique_ptr<std::istream> get(const std::string& key) override;
        void remove(const std::string& key) override;
        bool exists(const std::string& key) override;

    private:
        std::string make_key();

        S3Config cfg_;

        // provider_ должен жить дольше client_
        std::unique_ptr<minio::creds::Provider> provider_;
        std::unique_ptr<minio::s3::Client>      client_;
    };

}
