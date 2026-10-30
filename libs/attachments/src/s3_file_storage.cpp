#include "s3_file_storage.hpp"
#include "kb/common/uuid.hpp"

#include <openssl/sha.h>

#include <sstream>
#include <string>
#include <vector>

namespace kb::attachments {
    namespace {

        std::string sha256_hex(const std::string& data) {
            unsigned char hash[SHA256_DIGEST_LENGTH];
            SHA256(reinterpret_cast<const unsigned char*>(data.data()),
                   data.size(), hash);
            static const char* hex = "0123456789abcdef";
            std::string out;
            out.reserve(SHA256_DIGEST_LENGTH * 2);
            for (auto b : hash) {
                out.push_back(hex[b >> 4]);
                out.push_back(hex[b & 0xF]);
            }
            return out;
        }

    }

    S3FileStorage::S3FileStorage(S3Config cfg) : cfg_(std::move(cfg)) {
        minio::s3::BaseUrl url(cfg_.endpoint);
        url.https = cfg_.use_ssl;

        provider_ = std::make_unique<minio::creds::StaticProvider>(
            cfg_.access_key, cfg_.secret_key);

        client_ = std::make_unique<minio::s3::Client>(url, provider_.get());

        // Создать bucket, если отсутствует
        minio::s3::BucketExistsArgs be;
        be.bucket = cfg_.bucket;
        auto be_resp = client_->BucketExists(be);


        if (!be_resp) {
            throw AttachmentError(ErrorCode::StorageUnavailable,
                                  "S3 BucketExists failed: " +
                                  be_resp.Error().String());
        }
        if (!be_resp.exist) {
            minio::s3::MakeBucketArgs mb;
            mb.bucket = cfg_.bucket;
            auto mb_resp = client_->MakeBucket(mb);
            if (!mb_resp) {
                throw AttachmentError(ErrorCode::StorageUnavailable,
                                      "S3 MakeBucket failed: " +
                                      mb_resp.Error().String());
            }
        }
    }

    std::string S3FileStorage::make_key() {
        auto uuid = kb::common::generate_uuid();
        std::string clean;
        clean.reserve(32);
        for (char c : uuid) if (c != '-') clean.push_back(c);
        return clean.substr(0, 2) + "/" + clean.substr(2, 2) + "/" + uuid;
    }

    StoredObject S3FileStorage::put(const std::string& /*original_name*/,
                                    std::istream& data,
                                    std::uint64_t expected_size)
    {
        std::string buf((std::istreambuf_iterator<char>(data)),
                         std::istreambuf_iterator<char>());
        if (buf.size() != expected_size) {
            throw AttachmentError(ErrorCode::ValidationError,
                                  "size mismatch with declared");
        }

        const std::string key      = make_key();
        const std::string checksum = sha256_hex(buf);

        std::stringstream ss(buf);
        ss.seekg(0);

        minio::s3::PutObjectArgs args;
        args.bucket        = cfg_.bucket;
        args.object        = key;
        args.stream        = &ss;
        args.object_size   = buf.size();
        args.part_size     = 5 * 1024 * 1024;
        args.content_type  = "application/octet-stream";

        auto resp = client_->PutObject(args);
        if (!resp) {
            throw AttachmentError(ErrorCode::StorageUnavailable,
                                  "S3 PutObject failed: " +
                                  resp.Error().String());
        }

        return StoredObject{ key, buf.size(), checksum };
    }

    std::unique_ptr<std::istream> S3FileStorage::get(const std::string& key) {
        minio::s3::GetObjectArgs args;
        args.bucket = cfg_.bucket;
        args.object = key;

        std::string body;
        args.datafunc = [&body](minio::http::DataFunctionArgs a) -> bool {
            body.append(a.datachunk);
            return true;
        };

        auto resp = client_->GetObject(args);
        if (!resp) {
            throw AttachmentError(ErrorCode::StorageUnavailable,
                                  "S3 GetObject failed: " +
                                  resp.Error().String());
        }
        return std::make_unique<std::istringstream>(std::move(body));
    }

    void S3FileStorage::remove(const std::string& key) {
        minio::s3::RemoveObjectArgs args;
        args.bucket = cfg_.bucket;
        args.object = key;

        auto resp = client_->RemoveObject(args);
        if (!resp) {
            throw AttachmentError(ErrorCode::StorageUnavailable,
                                  "S3 RemoveObject failed: " +
                                  resp.Error().String());
        }
    }

    bool S3FileStorage::exists(const std::string& key) {
        minio::s3::StatObjectArgs args;
        args.bucket = cfg_.bucket;
        args.object = key;

        auto resp = client_->StatObject(args);
        return static_cast<bool>(resp);
    }
}