#pragma once
#include "attachment.hpp"
#include "i_file_storage.hpp"
#include "i_attachment_repository.hpp"
#include "i_search_index.hpp"
#include <vector>
#include <memory>
#include <string>

namespace kb::attachments {

    struct AttachmentConfig {
        std::uint64_t max_file_size_bytes = 50ull * 1024 * 1024;
        std::vector<std::string> allowed_mime_types = {
            "text/plain", "text/markdown",
            "application/pdf",
            "application/vnd.openxmlformats-officedocument.wordprocessingml.document", // docx
        };
    };

    struct UploadResult {
        AttachmentId id;
        std::string  storage_key;
        std::string  checksum_sha256;
        std::uint64_t size_bytes{0};
    };

    class AttachmentService {
    public:
        // Добавили search_index
        AttachmentService(std::shared_ptr<IFileStorage> storage,
                          std::shared_ptr<IAttachmentRepository> repo,
                          std::shared_ptr<kb::search::ISearchIndex> search_index,
                          AttachmentConfig cfg);

        UploadResult upload(const UploadRequest& req);

        std::optional<Attachment> get_metadata(const AttachmentId& id);
        std::vector<Attachment> list_for_page(const PageId& page_id);

        struct DownloadResult {
            Attachment metadata;
            std::unique_ptr<std::istream> stream;
        };
        DownloadResult download(const AttachmentId& id);

        void remove(const AttachmentId& id);

        std::vector<AttachmentId> search(const std::string& query, const std::string& page_id = "");

    private:
        void validate(const UploadRequest& req);
        void index_attachment(const AttachmentId& id,
                              const std::string& original_name,
                              const std::string& mime_type,
                              std::istream& data);

        std::shared_ptr<IFileStorage> storage_;
        std::shared_ptr<IAttachmentRepository> repo_;
        std::shared_ptr<kb::search::ISearchIndex> search_index_;
        AttachmentConfig cfg_;
    };

}