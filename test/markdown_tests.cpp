#include <gtest/gtest.h>
#include "markdown.h"
#include "markdown-tokens.h"

#include <sstream>
#include <fstream>

namespace {

std::string convert(const std::string &input) {
    markdown::Document doc;
    doc.read(input);
    std::ostringstream out;
    doc.write(out);
    return out.str();
}

} // namespace

TEST(LinkIdsUnit, StoresAndFindsUrlByCaseInsensitiveId) {
    markdown::LinkIds table;
    table.add("MyRef", "http://example.com", "a title");

    auto found = table.find("myref");
    ASSERT_TRUE(static_cast<bool>(found));
    EXPECT_EQ(found->url, "http://example.com");
    EXPECT_EQ(found->title, "a title");
}

TEST(LinkIdsUnit, ReturnsNoneForUnknownId) {
    markdown::LinkIds table;
    EXPECT_FALSE(static_cast<bool>(table.find("nonexistent")));
}

TEST(IsValidTagUnit, RecognizesBlockTag) {
    EXPECT_EQ(markdown::token::isValidTag("div"), 2u);
}

TEST(IsValidTagUnit, RecognizesInlineTag) {
    EXPECT_EQ(markdown::token::isValidTag("em"), 1u);
}

TEST(IsValidTagUnit, RejectsUnknownTag) {
    EXPECT_EQ(markdown::token::isValidTag("notarealtag"), 0u);
}

TEST(MarkdownConversion, PlainParagraph) {
    EXPECT_EQ(convert("Hello world"), "<p>Hello world</p>\n\n");
}

TEST(MarkdownConversion, AtxHeadingLevels) {
    EXPECT_EQ(convert("# Heading 1"), "<h1>Heading 1</h1>\n");
    EXPECT_EQ(convert("###### Heading 6"), "<h6>Heading 6</h6>\n");
}

TEST(MarkdownConversion, UnderlinedHeadingStyle) {
    EXPECT_EQ(convert("Title\n====="), "<h1>Title</h1>\n");
    EXPECT_EQ(convert("Subtitle\n--------"), "<h2>Subtitle</h2>\n");
}

TEST(MarkdownConversion, BoldAndItalicEmphasis) {
    EXPECT_EQ(convert("*italic*"), "<p><em>italic</em></p>\n\n");
    EXPECT_EQ(convert("**bold**"), "<p><strong>bold</strong></p>\n\n");
    EXPECT_EQ(convert("***both***"), "<p><strong><em>both</em></strong></p>\n\n");
}

TEST(MarkdownConversion, InlineCodeSpan) {
    EXPECT_EQ(convert("Use `printf()` here"),
              "<p>Use <code>printf()</code> here</p>\n\n");
}

TEST(MarkdownConversion, UnorderedList) {
    EXPECT_EQ(convert("- one\n- two"),
              "\n<ul>\n<li>one</li>\n<li>two</li>\n</ul>\n\n");
}

TEST(MarkdownConversion, OrderedList) {
    EXPECT_EQ(convert("1. first\n2. second"),
              "<ol>\n<li>first</li>\n<li>second</li>\n</ol>\n\n");
}

TEST(MarkdownConversion, InlineLink) {
    EXPECT_EQ(convert("[Example](http://example.com)"),
              "<p><a href=\"http://example.com\">Example</a></p>\n\n");
}

TEST(MarkdownConversion, ReferenceStyleLink) {
    std::string result = convert("[Example][1]\n\n[1]: http://example.com");
    EXPECT_NE(result.find("<a href=\"http://example.com\">Example</a>"), std::string::npos);
}

TEST(MarkdownConversion, ReferenceLinkTitleOnSeparateLine) {
    std::string result = convert("[Example][1]\n\n[1]: http://example.com\n\"a title\"");
    EXPECT_NE(result.find("href=\"http://example.com\""), std::string::npos);
    EXPECT_NE(result.find("title=\"a title\""), std::string::npos);
}

TEST(MarkdownConversion, HorizontalRule) {
    EXPECT_EQ(convert("---"), "<hr/>");
}

TEST(MarkdownConversion, EscapedCharactersNotTreatedAsMarkup) {
    std::string result = convert("\\*not italic\\*");
    EXPECT_EQ(result.find("<em>"), std::string::npos);
    EXPECT_NE(result.find("*not italic*"), std::string::npos);
}

TEST(MarkdownConversion, TwoTrailingSpacesForceLineBreak) {
    std::string result = convert("line one  \nline two");
    EXPECT_NE(result.find("<br/>"), std::string::npos);
}

TEST(MarkdownConversion, CrlfAndCrOnlyMatchLf) {
    EXPECT_EQ(convert("# Heading\r\n"), convert("# Heading\n"));
    EXPECT_EQ(convert("# Heading\r"), convert("# Heading\n"));
}

TEST(MarkdownConversion, HtmlSpecialCharactersEncoded) {
    std::string result = convert("A & B < C");
    EXPECT_NE(result.find("&amp;"), std::string::npos);
    EXPECT_NE(result.find("&lt;"), std::string::npos);
}

TEST(MarkdownConversion, CodeBlockViaIndent) {
    EXPECT_EQ(convert("    int x = 5;"), "<pre><code>int x = 5;\n</code></pre>\n\n");
}

TEST(MarkdownConversion, TabExpansionInLeadingWhitespace) {
    EXPECT_EQ(convert("\tint x = 5;"), "<pre><code>int x = 5;\n</code></pre>\n\n");
}

TEST(MarkdownConversion, ImageSyntax) {
    std::string result = convert("![alt text](image.png)");
    EXPECT_NE(result.find("<img src=\"image.png\""), std::string::npos);
    EXPECT_NE(result.find("alt=\"alt text\""), std::string::npos);
}

TEST(MarkdownConversion, ImageWithTitle) {
    std::string result = convert("![alt](image.png \"a title\")");
    EXPECT_NE(result.find("title=\"a title\""), std::string::npos);
}

TEST(MarkdownConversion, AutoLinkUrl) {
    std::string result = convert("<http://example.com>");
    EXPECT_NE(result.find("<a href=\"http://example.com\">"), std::string::npos);
}

TEST(MarkdownConversion, AutoLinkEmailIsObfuscated) {
    std::string result = convert("<someone@example.com>");
    EXPECT_NE(result.find("<a href=\"&#"), std::string::npos);
    EXPECT_NE(result.find("</a>"), std::string::npos);
}

TEST(MarkdownConversion, EmbeddedRawHtmlBlock) {
    std::string result = convert("<div>\nraw html content\n</div>");
    EXPECT_NE(result.find("<div>"), std::string::npos);
    EXPECT_NE(result.find("raw html content"), std::string::npos);
}

TEST(MarkdownConversion, HtmlCommentPassthrough) {
    std::string result = convert("<!-- a comment -->");
    EXPECT_NE(result.find("<!--"), std::string::npos);
}

TEST(MarkdownConversion, MultiLineBlockquoteContinuation) {
    std::string result = convert("> line one\n> line two\n> line three");
    EXPECT_NE(result.find("<blockquote>"), std::string::npos);
    EXPECT_NE(result.find("line one"), std::string::npos);
    EXPECT_NE(result.find("line three"), std::string::npos);
}

TEST(MarkdownConversion, ListInsideBlockquote) {
    std::string result = convert("> - item one\n> - item two");
    EXPECT_NE(result.find("<blockquote>"), std::string::npos);
    EXPECT_NE(result.find("<ul>"), std::string::npos);
    EXPECT_NE(result.find("<li>item one</li>"), std::string::npos);
}

TEST(MarkdownConversion, NestedSubList) {
    std::string result = convert("- outer one\n    - inner one\n    - inner two\n- outer two");
    EXPECT_NE(result.find("<ul>"), std::string::npos);
    EXPECT_NE(result.find("outer one"), std::string::npos);
    EXPECT_NE(result.find("inner one"), std::string::npos);
}

TEST(MarkdownEdgeCases, EmptyInputProducesEmptyOutput) {
    EXPECT_EQ(convert(""), "");
}

TEST(MarkdownEdgeCases, UnmatchedEmphasisMarkerIsLiteral) {
    std::string result = convert("this * is not emphasis");
    EXPECT_EQ(result.find("<em>"), std::string::npos);
}

TEST(MarkdownEdgeCases, DebugTokenOutputModeDoesNotCrash) {
    markdown::Document doc;
    doc.read("# Heading\n\nSome *text*.");
    std::ostringstream out;
    doc.writeTokens(out);
    EXPECT_FALSE(out.str().empty());
}

TEST(MarkdownKnownLimitations, EmphasisInsideLinkTextIsNotProcessed) {
    std::string result = convert("[**bold link**](https://github.com/Apollo87z/cpp-markdown)");
    EXPECT_NE(result.find("<a href=\"https://github.com/Apollo87z/cpp-markdown\">**bold link**</a>"), std::string::npos);
    EXPECT_EQ(result.find("<strong>"), std::string::npos);
}

TEST(MarkdownKnownLimitations, ListItemStartingWithEmphasisMarkerFailsToParse) {
    std::string result = convert("- *item one*\n- item two");
    EXPECT_EQ(result.find("<ul>"), std::string::npos);
}

TEST(MarkdownConversion, FullReadmeRegressionSnapshot) {
    std::ifstream readme("../README.md");
    ASSERT_TRUE(readme.good());
    std::ostringstream buf;
    buf << readme.rdbuf();

    std::string html = convert(buf.str());

    EXPECT_NE(html.find("<h1>Cpp-Markdown</h1>"), std::string::npos);
    EXPECT_NE(html.find("<h3>General</h3>"), std::string::npos);
    EXPECT_NE(html.find("Cpp-Markdown is released under the MIT license"), std::string::npos);
}