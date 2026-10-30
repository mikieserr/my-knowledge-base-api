#include <gtest/gtest.h>
#include "mocks/mock_file_storage.hpp"
#include "mocks/mock_attachment_repository.hpp"
#include "mocks/mock_search_index.hpp"
#include "attachment_service.hpp"
#include "errors.hpp"
#include <sstream>

using namespace kb::attachments;
using namespace kb::attachments::test;

namespace {

struct Fixture {
    std::shared_ptr<MockFileStorage>          storage;
    std::shared_ptr<MockAttachmentRepository> repo;
    std::shared_ptr<MockSearchIndex>          search_index;
    std::shared_ptr<AttachmentService>        svc;

    Fixture(AttachmentConfig cfg = {}) {
        storage = std::make_shared<MockFileStorage>();
        repo    = std::make_shared<MockAttachmentRepository>();
        search_index = std::make_shared<MockSearchIndex>();

        svc = std::make_shared<AttachmentService>(storage, repo, search_index, cfg);
    }

    static UploadRequest make_request(const std::string& content,
                                      const std::string& name = "test.txt",
                                      const std::string& mime = "text/plain",
                                      PageId page = "page-1",
                                      UserId user = "user-1") {
        static std::vector<std::unique_ptr<std::istringstream>> keepalive;
        auto stream = std::make_unique<std::istringstream>(content);
        std::istringstream* raw = stream.get();
        keepalive.push_back(std::move(stream));

        UploadRequest r;
        r.page_id       = page;
        r.user_id       = user;
        r.original_name = name;
        r.mime_type     = mime;
        r.size_bytes    = content.size();
        r.stream        = raw;
        return r;
    }
};

}

// Тест 1: Успешная загрузка сохраняет файл в S3 и метаданные в репозиторий
TEST(AttachmentTest, UploadSuccessSavesFileAndMetadata) {
    Fixture f;
    auto req = Fixture::make_request("hello world", "notes.txt");
    auto res = f.svc->upload(req);

    // Проверяем, что сервис вернул непустые идентификаторы и правильный размер
    ASSERT_FALSE(res.id.empty());
    ASSERT_FALSE(res.storage_key.empty());
    ASSERT_EQ(res.size_bytes, 11);

    // Проверяем, что файл физически попал в хранилище
    ASSERT_EQ(f.storage->count(), 1);
    ASSERT_EQ(f.storage->content(res.storage_key), "hello world");

    // Проверяем, что метаданные сохранились корректно
    auto meta = f.repo->find(res.id);
    ASSERT_TRUE(meta.has_value()); // Метаданные найдены
    EXPECT_EQ(meta->original_name, "notes.txt");
    EXPECT_EQ(meta->mime_type, "text/plain");
    EXPECT_EQ(meta->page_id, "page-1");
    EXPECT_EQ(meta->uploaded_by, "user-1");
}

// Тест 2: Успешная загрузка индексирует текст в Postgres (репозиторий)
TEST(AttachmentTest, UploadSuccessIndexesText) {
    Fixture f;
    auto req = Fixture::make_request("database backup instructions", "backup.md", "text/markdown");
    auto res = f.svc->upload(req);

    // Проверяем, что текст извлечён и сохранён в индекс
    ASSERT_TRUE(f.repo->has_index(res.id));
    ASSERT_EQ(f.repo->indexed_text(res.id), "database backup instructions");
}

// Тест 3: Скачивание возвращает метаданные и содержимое файла
TEST(AttachmentTest, DownloadReturnsMetadataAndContent) {
    Fixture f;
    auto req = Fixture::make_request("payload");
    auto up  = f.svc->upload(req);

    auto dl = f.svc->download(up.id);
    ASSERT_EQ(dl.metadata.id, up.id); // ID совпадает

    // Проверяем, что содержимое побайтно совпадает с исходным
    std::string body((std::istreambuf_iterator<char>(*dl.stream)), {});
    ASSERT_EQ(body, "payload");
}


// Тест 4: Удаление удаляет и файл из S3, и метаданные
TEST(AttachmentTest, RemoveDeletesFileAndMetadata) {
    Fixture f;
    auto res = f.svc->upload(Fixture::make_request("to be deleted"));

    // До удаления: файл и метаданные на месте
    ASSERT_EQ(f.storage->count(), 1);
    ASSERT_EQ(f.repo->count(), 1);

    f.svc->remove(res.id);

    // После удаления: пусто
    ASSERT_EQ(f.storage->count(), 0);
    ASSERT_EQ(f.repo->count(), 0);
}

// Тест 5: Получение списка вложений возвращает только файлы конкретной страницы
TEST(AttachmentTest, ListForPageReturnsOnlyAttachmentsOfPage) {
    Fixture f;
    f.svc->upload(Fixture::make_request("a", "a.txt", "text/plain", "page-1"));
    f.svc->upload(Fixture::make_request("b", "b.txt", "text/plain", "page-1"));
    f.svc->upload(Fixture::make_request("c", "c.txt", "text/plain", "page-2"));

    auto list = f.svc->list_for_page("page-1");
    ASSERT_EQ(list.size(), 2); // Только 2 файла принадлежат page-1
}


// Тест 6: Загрузка индексирует текст в поисковом движке (Manticore)
TEST(AttachmentTest, UploadIndexesInSearch) {
    Fixture f;
    auto req = Fixture::make_request("database backup instructions", "backup.md", "text/markdown");
    auto res = f.svc->upload(req);

    // Проверяем, что текст улетел в поисковый индекс
    ASSERT_TRUE(f.search_index->has_index(res.id));
    ASSERT_EQ(f.search_index->indexed_text(res.id), "database backup instructions");

    // Проверяем, что поиск по слову находит этот ID
    auto search_res = f.svc->search("backup");
    ASSERT_EQ(search_res.size(), 1);
    ASSERT_EQ(search_res[0], res.id);
}

// Тест 7: Пустое имя файла отклоняется
TEST(AttachmentTest, UploadEmptyNameThrows) {
    Fixture f;
    auto req = Fixture::make_request("content", "");
    ASSERT_THROW(f.svc->upload(req), AttachmentError);
}

// Тест 8: Пустой файл отклоняется
TEST(AttachmentTest, UploadEmptyFileThrows) {
    Fixture f;
    auto req = Fixture::make_request("", "empty.txt");
    ASSERT_THROW(f.svc->upload(req), AttachmentError);
}

// Тест 9: Файл, превышающий лимит, отклоняется с ошибкой FileTooLarge
TEST(AttachmentTest, UploadFileTooLargeThrows) {
    AttachmentConfig cfg;
    cfg.max_file_size_bytes = 4; // Ставим маленький лимит
    Fixture f(cfg);

    auto req = Fixture::make_request("too much data", "big.txt");
    try {
        f.svc->upload(req);
        FAIL() << "expected FileTooLarge";
    } catch (const AttachmentError& e) {
        ASSERT_EQ(e.code(), ErrorCode::FileTooLarge);
    }
}

// Тест 10: Запрещённый MIME-тип отклоняется
TEST(AttachmentTest, UploadUnsupportedMimeTypeThrows) {
    Fixture f;
    auto req = Fixture::make_request("#!/bin/sh\nrm -rf /", "evil.sh", "application/x-sh");
    try {
        f.svc->upload(req);
        FAIL() << "expected UnsupportedMimeType";
    } catch (const AttachmentError& e) {
        ASSERT_EQ(e.code(), ErrorCode::UnsupportedMimeType);
    }
}

// Тест 11: Попытка Path Traversal в имени файла отклоняется
TEST(AttachmentTest, UploadPathTraversalThrows) {
    Fixture f;
    for (auto& name : {"../etc/passwd", "..\\windows\\system32", "sub/dir/file.txt"}) {
        auto req = Fixture::make_request("x", name);
        try {
            f.svc->upload(req);
            FAIL() << "expected ValidationError for " << name;
        } catch (const AttachmentError& e) {
            ASSERT_EQ(e.code(), ErrorCode::ValidationError);
        }
    }
}

// Тест 12: Скачивание несуществующего вложения возвращает NotFound
TEST(AttachmentTest, DownloadNonExistentThrowsNotFound) {
    Fixture f;
    try {
        f.svc->download("no-such-id");
        FAIL() << "expected NotFound";
    } catch (const AttachmentError& e) {
        ASSERT_EQ(e.code(), ErrorCode::NotFound);
    }
}

// Тест 13: Сбой хранилища при загрузке возвращает StorageUnavailable
TEST(AttachmentTest, UploadStorageFailureThrows) {
    Fixture f;
    f.storage->fail_on_put = true; // Включаем режим сбоя в моке

    auto req = Fixture::make_request("data", "x.txt");
    try {
        f.svc->upload(req);
        FAIL() << "expected StorageUnavailable";
    } catch (const AttachmentError& e) {
        ASSERT_EQ(e.code(), ErrorCode::StorageUnavailable);
    }

    // Метаданные не должны сохраниться при ошибке
    ASSERT_EQ(f.repo->count(), 0);
}
