#pragma once
#include <istream>
#include <string>

namespace kb::attachments {
    std::string extract_text(const std::string& filename,
                             const std::string& mime_type,
                             std::istream& data);
}