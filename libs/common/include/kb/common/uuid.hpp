#pragma once

#include <string>

namespace kb::common {

    std::string generate_uuid();
    // Возвращает UUID версии 4 (например, 4f81c0e7-3c5a-4b2e-9d1f-8a7b6c5d4e3f)
    // и спользуется как уникальный идентификатор сущностей: вложение, страница, версия, сессия, событие аудита и т.д.
}