#pragma once
#include <string>
#include <vector>

namespace kb::search {
    class ISearchIndex {
    public:
        virtual ~ISearchIndex() = default;
        virtual void index(const std::string& id, const std::string& page_id, const std::string& text) = 0;
        virtual std::vector<std::string> search(const std::string& query, const std::string& page_id = "") = 0;
        virtual void remove(const std::string& id) = 0;
    };
}