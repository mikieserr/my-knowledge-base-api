#pragma once
#include "attachment.hpp"
#include <optional>
#include <vector>

namespace kb::attachments {

    class IAttachmentRepository {
    public:
        virtual ~IAttachmentRepository() = default;

        virtual AttachmentId create(const Attachment& a) = 0;
        virtual std::optional<Attachment> find(const AttachmentId& id) = 0;
        virtual std::vector<Attachment> list_by_page(const PageId& page_id) = 0;
        virtual void remove(const AttachmentId& id) = 0;
        virtual void save_indexed_text(const AttachmentId& id,
                                       const std::string& text) = 0;
    };

}