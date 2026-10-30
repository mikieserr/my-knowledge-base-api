#include "s3_file_storage.hpp"
#include "manticore_search_index.hpp"
#include "attachment_service.hpp"
#include "errors.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <memory>

using namespace kb::attachments;
using namespace kb::search;

namespace {

std::string env_or(const char* key, const char* fallback) {
    const char* v = std::getenv(key);
    return v ? std::string(v) : std::string(fallback);
}

std::string guess_mime(const std::string& filename) {
    auto dot = filename.find_last_of('.');
    if (dot == std::string::npos) return "application/octet-stream";
    std::string ext = filename.substr(dot + 1);
    if (ext == "txt")  return "text/plain";
    if (ext == "md")   return "text/markdown";
    if (ext == "pdf")  return "application/pdf";
    if (ext == "docx") return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    return "application/octet-stream";
}

// Заглушка репозитория для sandbox, чтобы не поднимать Postgres
class InMemoryRepo : public IAttachmentRepository {
public:
    AttachmentId create(const Attachment& a) override {
        by_id_[a.id] = a;
        return a.id;
    }
    std::optional<Attachment> find(const AttachmentId& id) override {
        if (by_id_.count(id)) return by_id_[id];
        return std::nullopt;
    }
    std::vector<Attachment> list_by_page(const PageId& page_id) override {
        return {};
    }
    void remove(const AttachmentId& id) override { by_id_.erase(id); }
    void save_indexed_text(const AttachmentId&, const std::string&) override {}
private:
    std::map<AttachmentId, Attachment> by_id_;
};

}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " [--full] <path-to-file>\n";
        return 1;
    }

    bool full_mode = false;
    std::string path;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--full") full_mode = true;
        else path = argv[i];
    }

    if (path.empty()) { std::cerr << "No file provided.\n"; return 1; }

    std::ifstream file(path, std::ios::binary);
    if (!file) { std::cerr << "Cannot open file: " << path << "\n"; return 2; }

    std::stringstream buf;
    buf << file.rdbuf();
    const std::string content = buf.str();

    std::string filename = path;
    if (auto pos = path.find_last_of("/\\"); pos != std::string::npos)
        filename = path.substr(pos + 1);

    S3Config s3_cfg{
        .endpoint   = env_or("S3_ENDPOINT",   "http://localhost:9000"),
        .access_key = env_or("S3_ACCESS_KEY", "kbadmin"),
        .secret_key = env_or("S3_SECRET_KEY", "kbsecret"),
        .bucket     = env_or("S3_BUCKET",     "kb-attachments"),
        .use_ssl    = false
    };

    try {
        auto s3 = std::make_shared<S3FileStorage>(s3_cfg);

        if (!full_mode) {
            // Режим только S3
            std::istringstream in(content);
            auto obj = s3->put(filename, in, content.size());
            std::cout << "Uploaded to S3 only: " << obj.key << "\n";
        } else {
            // S3 + Manticore + заглушка Postgres
            auto manticore = std::make_shared<ManticoreSearchIndex>(
                env_or("MANTICORE_URL", "http://localhost:9308")
            );
            auto repo = std::make_shared<InMemoryRepo>();

            AttachmentService svc(s3, repo, manticore, AttachmentConfig{});

            UploadRequest req;
            req.page_id       = "sandbox-page";
            req.user_id       = "sandbox-user";
            req.original_name = filename;
            req.mime_type     = guess_mime(filename);
            req.size_bytes    = content.size();

            std::istringstream in(content);
            req.stream = &in;

            auto res = svc.upload(req);
            std::cout << "Uploaded (full pipeline): " << res.id << "\n";
            std::cout << "Try searching in Manticore for words from this file.\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 3;
    }

    return 0;
}





// #include "s3_file_storage.hpp"
// #include "errors.hpp"
//
// #include <cstdlib>
// #include <fstream>
// #include <iostream>
// #include <sstream>
// #include <string>
//
// using namespace kb::attachments;
//
// namespace {
//
// std::string env_or(const char* key, const char* fallback) {
//     const char* v = std::getenv(key);
//     return v ? std::string(v) : std::string(fallback);
// }
//
// std::string guess_mime(const std::string& filename) {
//     auto dot = filename.find_last_of('.');
//     if (dot == std::string::npos) return "application/octet-stream";
//     std::string ext = filename.substr(dot + 1);
//     if (ext == "txt")  return "text/plain";
//     if (ext == "md")   return "text/markdown";
//     if (ext == "json") return "application/json";
//     if (ext == "png")  return "image/png";
//     if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
//     if (ext == "pdf")  return "application/pdf";
//     return "application/octet-stream";
// }
//
// }
//
// int main(int argc, char** argv) {
//     if (argc < 2) {
//         std::cerr << "Usage: " << argv[0] << " <path-to-file>\n";
//         return 1;
//     }
//
//     const std::string path = argv[1];
//
//     // Открываем файл как поток
//     std::ifstream file(path, std::ios::binary);
//     if (!file) {
//         std::cerr << "Cannot open file: " << path << "\n";
//         return 2;
//     }
//
//     // Читаем в память
//     std::stringstream buf;
//     buf << file.rdbuf();
//     const std::string content = buf.str();
//
//     // Имя файла из пути
//     std::string filename = path;
//     if (auto pos = path.find_last_of("/\\"); pos != std::string::npos)
//         filename = path.substr(pos + 1);
//
//     S3Config cfg{
//         .endpoint   = env_or("S3_ENDPOINT",   "http://localhost:9000"),
//         .access_key = env_or("S3_ACCESS_KEY", "kbadmin"),
//         .secret_key = env_or("S3_SECRET_KEY", "kbsecret"),
//         .bucket     = env_or("S3_BUCKET",     "kb-attachments"),
//         .use_ssl    = false
//     };
//
//     try {
//         S3FileStorage s3(cfg);
//
//         std::istringstream in(content);
//         auto obj = s3.put(filename, in, content.size());
//
//         std::cout << "Uploaded: " << filename << "\n"
//                   << "  mime:     " << guess_mime(filename) << "\n"
//                   << "  key:      " << obj.key << "\n"
//                   << "  size:     " << obj.size << "\n"
//                   << "  checksum: " << obj.checksum_sha256 << "\n"
//                   << "\nOpen http://localhost:9001/browser/"
//                   << cfg.bucket << " to see it.\n";
//     } catch (const AttachmentError& e) {
//         std::cerr << "AttachmentError: " << e.what() << "\n";
//         return 3;
//     }
//
//     return 0;
// }




#include "s3_file_storage.hpp"
#include "errors.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

// using namespace kb::attachments;
//
// namespace {
//
// std::string env_or(const char* key, const char* fallback) {
//     const char* v = std::getenv(key);
//     return v ? std::string(v) : std::string(fallback);
// }
//
// } // namespace
//
// int main() {
//     S3Config cfg{
//         .endpoint   = env_or("S3_ENDPOINT",   "http://localhost:9000"),
//         .access_key = env_or("S3_ACCESS_KEY", "kbadmin"),
//         .secret_key = env_or("S3_SECRET_KEY", "kbsecret"),
//         .bucket     = env_or("S3_BUCKET",     "kb-attachments"),
//         .use_ssl    = false
//     };
//
//     std::cout << "Config:\n"
//               << "  endpoint: " << cfg.endpoint << "\n"
//               << "  bucket:   " << cfg.bucket   << "\n\n";
//
//     try {
//         S3FileStorage s3(cfg);
//
//         // 1. PUT
//         std::string payload = "Hello from kb-attachments sandbox! backup instructions";
//         std::istringstream in(payload);
//
//         auto obj = s3.put("sandbox.txt", in, payload.size());
//         std::cout << "[PUT] ok\n"
//                   << "  key:      " << obj.key << "\n"
//                   << "  size:     " << obj.size << "\n"
//                   << "  checksum: " << obj.checksum_sha256 << "\n\n";
//
//         // 2. EXISTS
//         std::cout << "[EXISTS] " << (s3.exists(obj.key) ? "yes" : "no") << "\n\n";
//
//         // 3. GET
//         auto stream = s3.get(obj.key);
//         std::string got((std::istreambuf_iterator<char>(*stream)), {});
//         std::cout << "[GET] ok\n"
//                   << "  content:  " << got << "\n\n";
//
//         // 4. REMOVE — закомментируй строку ниже, если хочешь
//         //    увидеть файл в MinIO Web UI после запуска
//         // s3.remove(obj.key);
//         // std::cout << "[REMOVE] ok\n";
//
//         std::cout << "Done. Key = " << obj.key << "\n";
//         std::cout << "Open http://localhost:9001/browser/kb-attachments to see the file.\n";
//
//     } catch (const AttachmentError& e) {
//         std::cerr << "AttachmentError: " << e.what() << "\n";
//         return 1;
//     } catch (const std::exception& e) {
//         std::cerr << "Exception: " << e.what() << "\n";
//         return 2;
//     }
//     return 0;
// }