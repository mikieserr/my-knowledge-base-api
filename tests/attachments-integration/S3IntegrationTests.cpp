#include <gtest/gtest.h>
#include "s3_file_storage.hpp"

#include <cstdlib>
#include <sstream>

using namespace kb::attachments;

namespace {
    S3Config cfg_from_env() {
        return S3Config{
            .endpoint   = std::getenv("S3_ENDPOINT")   ? std::getenv("S3_ENDPOINT")   : "http://localhost:9000",
            .access_key = std::getenv("S3_ACCESS_KEY") ? std::getenv("S3_ACCESS_KEY") : "kbadmin",
            .secret_key = std::getenv("S3_SECRET_KEY") ? std::getenv("S3_SECRET_KEY") : "kbsecret",
            .bucket     = std::getenv("S3_BUCKET")     ? std::getenv("S3_BUCKET")     : "kb-attachments",
            .use_ssl    = false
        };
    }
}

// Тест 1: Полный цикл работы с S3 (загрузка, проверка, скачивание, удаление)
TEST(S3IntegrationTest, PutGetRemoveRoundtrip) {
    // Подключаемся к реальному MinIO/SILO
    S3FileStorage s3(cfg_from_env());

    std::string payload = "integration test payload";
    std::istringstream in(payload);

    // 1. Загружаем файл в S3
    auto obj = s3.put("hello.txt", in, payload.size());
    ASSERT_FALSE(obj.key.empty());           // Ключ сгенерирован
    ASSERT_EQ(obj.size, payload.size());     // Размер совпадает

    // 2. Проверяем, что файл реально существует
    ASSERT_TRUE(s3.exists(obj.key));

    // 3. Скачиваем и проверяем содержимое
    auto stream = s3.get(obj.key);
    std::string got((std::istreambuf_iterator<char>(*stream)), {});
    ASSERT_EQ(got, payload);                 // Содержимое побайтно совпадает

    // 4. Удаляем файл и проверяем, что его больше нет
    s3.remove(obj.key);
    ASSERT_FALSE(s3.exists(obj.key));
}