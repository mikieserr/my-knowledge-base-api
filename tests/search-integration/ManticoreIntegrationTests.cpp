#include <gtest/gtest.h>
#include "manticore_search_index.hpp"
#include <cstdlib>
#include <fstream>

using namespace kb::search;

namespace {
    std::string manticore_url() {
        return std::getenv("MANTICORE_URL") ? std::getenv("MANTICORE_URL") : "http://localhost:9308";
    }

    // Валидные UUID для тестов (v4)
    const std::string UUID_1 = "550e8400-e29b-41d4-a716-446655440001";
    const std::string UUID_2 = "550e8400-e29b-41d4-a716-446655440002";
}

// Тест 1: Индексация и поиск
TEST(ManticoreIntegrationTest, IndexAndSearch) {
    ManticoreSearchIndex index(manticore_url());

    // Очищаем перед тестом
    index.remove(UUID_1);

    index.index(UUID_1, "page-1", "This is a test document about backups");

    auto results = index.search("backups", "page-1");
    ASSERT_EQ(results.size(), 1);
    ASSERT_EQ(results[0], UUID_1); // Теперь это валидный UUID

    index.remove(UUID_1);
}

// Тест 2: Удаление из индекса
TEST(ManticoreIntegrationTest, RemoveFromIndex) {
    ManticoreSearchIndex index(manticore_url());

    index.remove(UUID_2);

    index.index(UUID_2, "page-1", "temporary text");
    auto before = index.search("temporary", "page-1");
    ASSERT_EQ(before.size(), 1);
    ASSERT_EQ(before[0], UUID_2);

    index.remove(UUID_2);
    auto after = index.search("temporary", "page-1");
    ASSERT_TRUE(after.empty());
}