#pragma once
#include "i_search_index.hpp"
#include <string>

namespace kb::search {
    class ManticoreSearchIndex : public ISearchIndex {
    public:
        explicit ManticoreSearchIndex(std::string base_url);
        void index(const std::string& id, const std::string& page_id, const std::string& text) override;
        std::vector<std::string> search(const std::string& query, const std::string& page_id = "") override;
        void remove(const std::string& id) override;
    private:
        std::string base_url_;
        std::string execute_sql(const std::string& sql);
        void ensure_table();
    };
}