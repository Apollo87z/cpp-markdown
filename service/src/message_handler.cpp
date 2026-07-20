#include "message_handler.h"
#include <sstream>


namespace {
std::optional<std::string> extractField(const std::string &raw, const std::string &key) {
    std::string pattern = "\"" + key + "\"";
    auto pos = raw.find(pattern);
    if (pos == std::string::npos) return std::nullopt;
    pos = raw.find(':', pos);
    if (pos == std::string::npos) return std::nullopt;
    pos = raw.find('"', pos);
    if (pos == std::string::npos) return std::nullopt;
    auto end = raw.find('"', pos + 1);
    if (end == std::string::npos) return std::nullopt;
    // Handle escaped newlines in the markdown field
    std::string value = raw.substr(pos + 1, end - pos - 1);
    std::string unescaped;
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '\\' && i + 1 < value.size() && value[i+1] == 'n') {
            unescaped += '\n';
            ++i;
        } else {
            unescaped += value[i];
        }
    }
    return unescaped;
}
}

std::optional<ParsedMessage> parseKafkaMessage(const std::string &raw) {
    if (raw.empty()) return std::nullopt;

    auto tenantId = extractField(raw, "tenant_id");
    auto documentId = extractField(raw, "document_id");
    auto markdown = extractField(raw, "markdown");

    if (!tenantId || tenantId->empty()) return std::nullopt;
    if (!documentId || documentId->empty()) return std::nullopt;
    if (!markdown) return std::nullopt;

    return ParsedMessage{*tenantId, *documentId, *markdown};
}

std::string buildResultMessage(const std::string &tenantId, const std::string &documentId, const std::string &html) {
    std::ostringstream out;
    out << "{\"tenant_id\":\"" << tenantId << "\","
        << "\"document_id\":\"" << documentId << "\","
        << "\"html\":\"";
    for (char c : html) {
        if (c == '\n') out << "\\n";
        else if (c == '"') out << "\\\"";
        else out << c;
    }
    out << "\"}";
    return out.str();
}