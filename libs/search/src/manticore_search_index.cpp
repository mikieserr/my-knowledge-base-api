#include "manticore_search_index.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <iostream>

namespace kb::search {
namespace {
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}
std::string escape_sql(const std::string& s) {
    std::string out;
    for (char c : s) { if (c == '\'') out += "\\'"; else out += c; }
    return out;
}
}

ManticoreSearchIndex::ManticoreSearchIndex(std::string base_url) : base_url_(std::move(base_url)) {
    ensure_table();
}

void ManticoreSearchIndex::ensure_table() {
    try {
        // Теперь Manticore поддерживает id uuid
        execute_sql("CREATE TABLE IF NOT EXISTS attachments (id uuid, page_id string attribute, content text)");
    } catch (const std::exception& e) {
        std::cerr << "Failed to ensure table: " << e.what() << "\n";
    }
}

std::string ManticoreSearchIndex::execute_sql(const std::string& sql) {
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl init failed");
    std::string readBuffer;
    std::string url = base_url_ + "/cli_json";

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: text/plain");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, sql.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    if (res != CURLE_OK) throw std::runtime_error(curl_easy_strerror(res));
    return readBuffer;
}

void ManticoreSearchIndex::index(const std::string& id, const std::string& page_id, const std::string& text) {
    std::string sql = "INSERT INTO attachments (id, page_id, content) VALUES ('"
                    + id + "', '" + page_id + "', '" + escape_sql(text) + "')";
    execute_sql(sql);
}

std::vector<std::string> ManticoreSearchIndex::search(const std::string& query, const std::string& page_id) {
    std::string sql = "SELECT id FROM attachments WHERE MATCH('" + escape_sql(query) + "')";
    if (!page_id.empty()) sql += " AND page_id = '" + page_id + "'";
    sql += " LIMIT 50";

    std::string response = execute_sql(sql);
    std::vector<std::string> ids;
    try {
        auto j = nlohmann::json::parse(response);
        if (j.is_array() && !j.empty() && j[0].contains("data")) {
            for (auto& item : j[0]["data"]) {
                if (item.contains("id")) {
                    if (item["id"].is_string()) {
                        ids.push_back(item["id"].get<std::string>());
                    } else {
                        ids.push_back(std::to_string(item["id"].get<long long>()));
                    }
                }
            }
        }
    } catch (...) { /* пустой результат или ошибка парсинга */ }
    return ids;
}

void ManticoreSearchIndex::remove(const std::string& id) {
    execute_sql("DELETE FROM attachments WHERE id = '" + id + "'");
}
}