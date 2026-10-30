#pragma once
#include "i_attachment_repository.hpp"
#include "errors.hpp"

#include <map>
#include <vector>
#include <string>

namespace kb::attachments::test {

    class MockAttachmentRepository : public IAttachmentRepository {
    public:
        AttachmentId create(const Attachment& a) override {
            Attachment copy = a;
            if (copy.id.empty())
                copy.id = "att-" + std::to_string(++counter_);
            by_id_[copy.id] = copy;
            created_ids_.push_back(copy.id);
            return copy.id;
        }

        std::optional<Attachment> find(const AttachmentId& id) override {
            auto it = by_id_.find(id);
            if (it == by_id_.end()) return std::nullopt;
            return it->second;
        }

        std::vector<Attachment> list_by_page(const PageId& page_id) override {
            std::vector<Attachment> out;
            for (auto& [_, a] : by_id_)
                if (a.page_id == page_id) out.push_back(a);
            return out;
        }

        void remove(const AttachmentId& id) override {
            by_id_.erase(id);
        }

        void save_indexed_text(const AttachmentId& id,
                               const std::string& text) override {
            indexed_[id] = text;
        }

        // --- хелперы для тестов ---
        const std::vector<AttachmentId>& created_ids() const { return created_ids_; }
        std::size_t count() const { return by_id_.size(); }
        bool has_index(const AttachmentId& id) const {
            return indexed_.count(id) > 0;
        }
        const std::string& indexed_text(const AttachmentId& id) const {
            return indexed_.at(id);
        }

    private:
        std::map<AttachmentId, Attachment> by_id_;
        std::map<AttachmentId, std::string> indexed_;
        std::vector<AttachmentId> created_ids_;
        std::size_t counter_{0};
    };

}