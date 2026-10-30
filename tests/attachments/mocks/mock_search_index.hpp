#pragma once

#include "i_search_index.hpp"
#include <map>
#include <vector>
#include <string>

namespace kb::attachments::test {

    class MockSearchIndex : public kb::search::ISearchIndex {
    public:
        void index(const std::string& id, const std::string& page_id, const std::string& text) override {
            data_[id] = {page_id, text};
        }

        std::vector<std::string> search(const std::string& query, const std::string& page_id = "") override {
            std::vector<std::string> result;
            for (const auto& [id, pair] : data_) {
                if ((page_id.empty() || pair.first == page_id) &&
                    pair.second.find(query) != std::string::npos) {
                    result.push_back(id);
                    }
            }
            return result;
        }

        void remove(const std::string& id) override {
            data_.erase(id);
        }

        // --- хелперы для тестов ---
        bool has_index(const std::string& id) const {
            return data_.count(id) > 0;
        }
        const std::string& indexed_text(const std::string& id) const {
            return data_.at(id).second;
        }
        std::size_t count() const { return data_.size(); }

    private:
        // id -> {page_id, text}
        std::map<std::string, std::pair<std::string, std::string>> data_;
    };

}