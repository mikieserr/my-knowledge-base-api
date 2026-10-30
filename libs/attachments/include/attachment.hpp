#pragma once

#include <cstdint>
#include <chrono>
#include <string>

namespace kb::attachments {

    using AttachmentId = std::string;   // UUID
    using PageId       = std::string;   // UUID
    using UserId       = std::string;   // UUID

    struct Attachment {
        AttachmentId id;
        PageId       page_id;
        std::string  original_name;    // имя от пользователя
        std::string  storage_key;      // внутренний путь в S3: "4f/81/<uuid>"
        std::string  mime_type;
        std::uint64_t size_bytes{0};
        std::string  checksum_sha256;
        UserId       uploaded_by;
        std::chrono::system_clock::time_point uploaded_at;
    };

    struct UploadRequest {
        PageId       page_id;
        UserId       user_id;
        std::string  original_name;
        std::string  mime_type;
        std::uint64_t size_bytes;
        std::istream* stream;
    };

}