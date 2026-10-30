#include "text_extractor.hpp"
#include <sstream>
#include <iostream>
#include <cstring>

#include <poppler/cpp/poppler-document.h> //для pdf
#include <poppler/cpp/poppler-page.h>

#include <zip.h> // для docx
#include <pugixml.hpp>

namespace kb::attachments {

namespace {

// Вспомогательная функция читает файл из ZIP-архива в память
std::string read_zip_file(const std::string& zip_data, const std::string& filename) {
    zip_error_t error;
    zip_error_init(&error);
    zip_source_t* src = zip_source_buffer_create(zip_data.data(), zip_data.size(), 0, &error);
    if (!src) { zip_error_fini(&error); return {}; }

    zip_t* za = zip_open_from_source(src, ZIP_RDONLY, &error);
    if (!za) { zip_source_free(src); zip_error_fini(&error); return {}; }

    zip_file_t* zf = zip_fopen(za, filename.c_str(), 0);
    if (!zf) { zip_close(za); zip_error_fini(&error); return {}; }

    std::string content;
    char buf[4096];
    zip_int64_t n;
    while ((n = zip_fread(zf, buf, sizeof(buf))) > 0) {
        content.append(buf, n);
    }
    zip_fclose(zf);
    zip_close(za);
    zip_error_fini(&error);
    return content;
}

// DOCX ищем все теги <w:t> (текст)
std::string extract_docx(const std::string& data) {
    std::string xml = read_zip_file(data, "word/document.xml");
    if (xml.empty()) return {};

    pugi::xml_document doc;
    if (!doc.load_string(xml.c_str())) return {};

    std::string text;
    // XPath для поиска всех текстовых узлов
    pugi::xpath_node_set nodes = doc.select_nodes("//*[local-name()='t']");
    for (const auto& node : nodes) {
        text += node.node().text().get();
        text += " "; // Разделяем слова
    }
    return text;
}

}

std::string extract_text(const std::string& filename,
                         const std::string& mime_type,
                         std::istream& data) {

    // 1. Текстовые файлы
    if (mime_type == "text/plain" || mime_type == "text/markdown") {
        std::ostringstream ss;
        ss << data.rdbuf();
        return ss.str();
    }

    // Читаем бинарные данные в память для дальнейшей обработки (PDF и DOCX)
    std::string binary_data((std::istreambuf_iterator<char>(data)),
                             std::istreambuf_iterator<char>());

    // 2. PDF
    if (mime_type == "application/pdf") {
        std::unique_ptr<poppler::document> doc(
            poppler::document::load_from_raw_data(binary_data.data(), binary_data.size())
        );
        if (!doc || doc->is_locked()) return {};

        std::string full_text;
        for (int i = 0; i < doc->pages(); ++i) {
            std::unique_ptr<poppler::page> page(doc->create_page(i));
            if (page) {
                // to_utf8() важен для корректной кириллицы, поэтому добавляем его через append с итераторами
                auto utf8_bytes = page->text().to_utf8();
                full_text.append(utf8_bytes.begin(), utf8_bytes.end());
                full_text += "\n";
            }
        }
        return full_text;
    }

    // 3. DOCX
    if (mime_type == "application/vnd.openxmlformats-officedocument.wordprocessingml.document") {
        return extract_docx(binary_data);
    }

    // Остальные форматы не поддерживаются
    return {};
}

}