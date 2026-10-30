#include <gtest/gtest.h>
#include "kb/common/uuid.hpp"
#include <set>

// Тест 1: Проверка длины и формата UUID
TEST(UuidTest, LengthAndFormat) {
    auto u = kb::common::generate_uuid();
    ASSERT_EQ(u.size(), 36);
    EXPECT_EQ(u[8], '-');
    EXPECT_EQ(u[13], '-');
    EXPECT_EQ(u[18], '-');
    EXPECT_EQ(u[23], '-');
}

// Тест 2: Проверка уникальности (10 000 вызовов — все разные)
TEST(UuidTest, Uniqueness) {
    std::set<std::string> seen;
    for (int i = 0; i < 10000; ++i) {
        auto u = kb::common::generate_uuid();
        // insert возвращает false, если элемент уже есть
        ASSERT_TRUE(seen.insert(u).second);
    }
}

// Тест 3: Проверка версии (4) и варианта (RFC 4122)
TEST(UuidTest, VersionAndVariant) {
    auto u = kb::common::generate_uuid();
    // 13-й символ (индекс 14) — версия, должен быть '4'
    ASSERT_EQ(u[14], '4');
    // 17-й символ (индекс 19) — вариант, должен быть 8/9/a/b
    ASSERT_NE(std::string("89ab").find(u[19]), std::string::npos);
}