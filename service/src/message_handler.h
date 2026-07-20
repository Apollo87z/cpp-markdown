#pragma once
#include <string>
#include <optional>

struct ParsedMessage {
    std::string tenantId;
    std::string documentId;
    std::string markdown;
};

std::optional<ParsedMessage> parseKafkaMessage(const std::string &raw);
std::string buildResultMessage(const std::string &tenantId, const std::string &documentId, const std::string &html);