#include "attachment_service.hpp"
#include "errors.hpp"
#include "text_extractor.hpp"
#include "kb/common/uuid.hpp"

#include <openssl/sha.h>

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

AttachmentService::AttachmentService(std::shared_ptr<IFileStorage> storage,
                                     std::shared_ptr<IAttachmentRepository> repo,
                                     std::shared_ptr<kb::search::ISearchIndex> search_index,
                                     AttachmentConfig cfg)
    : storage_(std::move(storage)),
      repo_(std::move(repo)),
      search_index_(std::move(search_index)),
      cfg_(std::move(cfg)) {}

void AttachmentService::validate(const UploadRequest& req) {
    if (req.original_name.empty())
        throw AttachmentError(ErrorCode::ValidationError, "empty file name");
    if (req.size_bytes == 0)
        throw AttachmentError(ErrorCode::ValidationError, "empty file");
    if (req.size_bytes > cfg_.max_file_size_bytes)
        throw AttachmentError(ErrorCode::FileTooLarge, "file too large");

    // защита от path traversal в имени
    if (req.original_name.find("..") != std::string::npos ||
        req.original_name.find('/') != std::string::npos ||
        req.original_name.find('\\') != std::string::npos)
        throw AttachmentError(ErrorCode::ValidationError, "invalid file name");

    bool mime_ok = false;
    for (auto& m : cfg_.allowed_mime_types)
        if (m == req.mime_type) { mime_ok = true; break; }
    if (!mime_ok)
        throw AttachmentError(ErrorCode::UnsupportedMimeType, "mime not allowed");
}

UploadResult AttachmentService::upload(const UploadRequest& req) {
    validate(req);

    // 1. Кладём в S3
    StoredObject obj = storage_->put(req.original_name, *req.stream,
                                     req.size_bytes);

    // 2. Метаданные
    Attachment a;
    a.id              = kb::common::generate_uuid();
    a.page_id         = req.page_id;
    a.original_name   = req.original_name;
    a.storage_key     = obj.key;
    a.mime_type       = req.mime_type;
    a.size_bytes      = obj.size;
    a.checksum_sha256 = obj.checksum_sha256;
    a.uploaded_by     = req.user_id;
    a.uploaded_at     = std::chrono::system_clock::now();

    // 3. Транзакция: метаданные
    repo_->create(a);

    // 4. Извлекаем текст и сохраняем в индекс (Postgres + Manticore)
    auto stream = storage_->get(a.storage_key);
    index_attachment(a.id, a.original_name, a.mime_type, *stream);

    return UploadResult{ a.id, a.storage_key, a.checksum_sha256, a.size_bytes };
}

void AttachmentService::index_attachment(const AttachmentId& id,
                                         const std::string& name,
                                         const std::string& mime,
                                         std::istream& data) {
    std::string text = extract_text(name, mime, data);
    if (!text.empty()) {
        // Сохраняем в Postgres
        repo_->save_indexed_text(id, text);

        // Отправляем в Manticore Search для быстрого поиска и передаем id, page_id и текст
        search_index_->index(id, "", text);
    }
}

AttachmentService::DownloadResult
AttachmentService::download(const AttachmentId& id) {
    auto meta = repo_->find(id);
    if (!meta)
        throw AttachmentError(ErrorCode::NotFound, "attachment not found");
    auto stream = storage_->get(meta->storage_key);
    if (!stream)
        throw AttachmentError(ErrorCode::StorageUnavailable, "cannot read file");
    return DownloadResult{ *meta, std::move(stream) };
}

void AttachmentService::remove(const AttachmentId& id) {
    auto meta = repo_->find(id);
    if (!meta) throw AttachmentError(ErrorCode::NotFound, "attachment not found");

    // 1. Удаляем из S3
    storage_->remove(meta->storage_key);

    // 2. Удаляем метаданные из Postgres
    repo_->remove(id);

    // 3. Удаляем из Manticore Search
    search_index_->remove(id);
}

std::optional<Attachment>
AttachmentService::get_metadata(const AttachmentId& id) {
    return repo_->find(id);
}

std::vector<Attachment>
AttachmentService::list_for_page(const PageId& page_id) {
    return repo_->list_by_page(page_id);
}

//Поиск
std::vector<AttachmentId> AttachmentService::search(const std::string& query, const std::string& page_id) {
    return search_index_->search(query, page_id);
}

}