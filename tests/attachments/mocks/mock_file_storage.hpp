#pragma once

#include "i_file_storage.hpp"
#include "errors.hpp"

#include <map>
#include <sstream>
#include <memory>
#include <string>

namespace kb::attachments::test {

// In-memory хранилище: ключ -> содержимое
class MockFileStorage : public IFileStorage {
public:
    // Поведение по умолчанию-- всё ок
    bool fail_on_put    = false;
    bool fail_on_get    = false;
    bool fail_on_remove = false;

    StoredObject put(const std::string& /*original_name*/,
                     std::istream& data,
                     std::uint64_t expected_size) override {
        if (fail_on_put)
            throw AttachmentError(ErrorCode::StorageUnavailable, "mock put failed");

        std::string body((std::istreambuf_iterator<char>(data)),
                          std::istreambuf_iterator<char>());

        if (body.size() != expected_size)
            throw AttachmentError(ErrorCode::ValidationError, "size mismatch");

        // Генерируем детерминированный ключ по счётчику
        std::string key = "test/" + std::to_string(++counter_);
        store_[key] = body;

        StoredObject obj;
        obj.key             = key;
        obj.size            = body.size();
        obj.checksum_sha256 = fake_sha256(body);
        return obj;
    }

    std::unique_ptr<std::istream> get(const std::string& key) override {
        if (fail_on_get)
            throw AttachmentError(ErrorCode::StorageUnavailable, "mock get failed");

        auto it = store_.find(key);
        if (it == store_.end())
            throw AttachmentError(ErrorCode::NotFound, "mock: key not found");

        return std::make_unique<std::istringstream>(it->second);
    }

    void remove(const std::string& key) override {
        if (fail_on_remove)
            throw AttachmentError(ErrorCode::StorageUnavailable, "mock remove failed");
        store_.erase(key);
    }

    bool exists(const std::string& key) override {
        return store_.count(key) > 0;
    }

    //хелперы для тестов
    std::size_t count() const { return store_.size(); }
    const std::string& content(const std::string& key) const {
        return store_.at(key);
    }

private:
    static std::string fake_sha256(const std::string& s) {
        // Не реальный SHA-256, только детерминированное значения
        std::size_t h = std::hash<std::string>{}(s);
        return "test_sha_" + std::to_string(h);
    }

    std::map<std::string, std::string> store_;
    std::size_t counter_{0};
};

}