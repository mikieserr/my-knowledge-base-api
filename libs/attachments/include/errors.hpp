#pragma once
#include <stdexcept>
#include <string>

namespace kb::attachments {

    enum class ErrorCode {
        ValidationError,
        FileTooLarge,
        UnsupportedMimeType,
        StorageUnavailable,
        NotFound,
        AccessDenied,
        InternalError
    };

    class AttachmentError : public std::runtime_error {
    public:
        AttachmentError(ErrorCode code, std::string msg)
            : std::runtime_error(std::move(msg)), code_(code) {}
        ErrorCode code() const noexcept { return code_; }
    private:
        ErrorCode code_;
    };

}