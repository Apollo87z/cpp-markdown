#include <gtest/gtest.h>
#include "message_handler.h"

TEST(MessageHandlerTest, ParsesValidMessage) {
    std::string raw = R"({"tenant_id":"acme","document_id":"doc-123","markdown":"# Hello"})";
    auto result = parseKafkaMessage(raw);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->tenantId, "acme");
    EXPECT_EQ(result->documentId, "doc-123");
    EXPECT_EQ(result->markdown, "# Hello");
}

TEST(MessageHandlerTest, HandlesEscapedNewlinesInMarkdown) {
    std::string raw = R"({"tenant_id":"acme","document_id":"doc-1","markdown":"# Hello\nWorld"})";
    auto result = parseKafkaMessage(raw);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->markdown, "# Hello\nWorld");
}

TEST(MessageHandlerTest, RejectsEmptyInput) {
    auto result = parseKafkaMessage("");
    EXPECT_FALSE(result.has_value());
}

TEST(MessageHandlerTest, RejectsMissingTenantId) {
    std::string raw = R"({"document_id":"doc-1","markdown":"# Hello"})";
    auto result = parseKafkaMessage(raw);
    EXPECT_FALSE(result.has_value());
}

TEST(MessageHandlerTest, RejectsMissingDocumentId) {
    std::string raw = R"({"tenant_id":"acme","markdown":"# Hello"})";
    auto result = parseKafkaMessage(raw);
    EXPECT_FALSE(result.has_value());
}

TEST(MessageHandlerTest, RejectsEmptyTenantId) {
    std::string raw = R"({"tenant_id":"","document_id":"doc-1","markdown":"# Hello"})";
    auto result = parseKafkaMessage(raw);
    EXPECT_FALSE(result.has_value());
}

TEST(MessageHandlerTest, AcceptsEmptyMarkdownField) {
    // Empty markdown is technically valid input to convertMarkdown() itself
    // (it just produces empty output), so the parser shouldn't reject it.
    std::string raw = R"({"tenant_id":"acme","document_id":"doc-1","markdown":""})";
    auto result = parseKafkaMessage(raw);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->markdown, "");
}

TEST(MessageHandlerTest, BuildsResultMessageCorrectly) {
    std::string result = buildResultMessage("acme", "doc-1", "<h1>Hello</h1>");
    EXPECT_EQ(result, R"({"tenant_id":"acme","document_id":"doc-1","html":"<h1>Hello</h1>"})");
}

TEST(MessageHandlerTest, BuildResultMessageEscapesNewlinesAndQuotes) {
    std::string result = buildResultMessage("acme", "doc-1", "<p>Line1\nLine2 \"quoted\"</p>");
    EXPECT_EQ(result, R"({"tenant_id":"acme","document_id":"doc-1","html":"<p>Line1\nLine2 \"quoted\"</p>"})");
}