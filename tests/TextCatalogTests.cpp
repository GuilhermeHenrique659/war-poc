#include "view/ui/TextCatalog.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

std::string completeCatalog()
{
    std::string content = "# test catalog\n\n";
    for (const auto& entry : view::ui::kTextKeys) {
        content += std::string(entry.key) + "=" + std::string(entry.key) + "\n";
    }
    return content;
}

class TextCatalogTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        path_ = std::filesystem::temp_directory_path() / "war_poc_text_catalog_test.txt";
        catalog_.clear();
    }

    void TearDown() override
    {
        std::error_code error;
        std::filesystem::remove(path_, error);
        catalog_.clear();
        view::ui::TextCatalog::instance().clear();
    }

    void write(std::string_view content) const
    {
        std::ofstream file(path_, std::ios::binary | std::ios::trunc);
        file.write(content.data(), static_cast<std::streamsize>(content.size()));
    }

    std::filesystem::path path_;
    view::ui::TextCatalog catalog_;
};

} // namespace

TEST_F(TextCatalogTest, LoadsValuesFromTheExternalFile)
{
    // Given a complete catalog whose menu title is overridden.
    std::string content = completeCatalog();
    const std::string generated = "menu.title=menu.title\n";
    const auto position = content.find(generated);
    ASSERT_NE(position, std::string::npos);
    content.replace(position, generated.size(), "menu.title=WAR\n");
    write(content);
    catalog_.loadFromFile(path_);

    // Then the loaded value is available.
    EXPECT_EQ(catalog_.text(view::ui::TextId::MenuTitle), "WAR");
}

TEST_F(TextCatalogTest, FormatsPlaceholdersAndCanReorderArguments)
{
    // Given a catalog entry that swaps its two placeholders.
    std::string content = completeCatalog();
    const std::string generated = "menu.title=menu.title\n";
    const auto position = content.find(generated);
    ASSERT_NE(position, std::string::npos);
    content.replace(position, generated.size(), "menu.title={1} {0}\n");
    write(content);
    view::ui::TextCatalog::instance().loadFromFile(path_);

    // When it is formatted.
    const std::string result = view::ui::format(view::ui::TextId::MenuTitle, "A", "B");

    // Then the arguments follow the template order.
    EXPECT_EQ(result, "B A");
}

TEST_F(TextCatalogTest, MissingFileThrows)
{
    // Given a path that does not exist.
    const std::filesystem::path missing = path_.parent_path() / "war_poc_missing_catalog.txt";

    // When it is loaded.
    // Then the failure is explicit.
    EXPECT_THROW(catalog_.loadFromFile(missing), std::runtime_error);
}

TEST_F(TextCatalogTest, MalformedLineThrows)
{
    // Given a catalog with a line that has no '='.
    write(completeCatalog() + "this line has no separator\n");

    // When it is loaded.
    // Then the failure names the problem instead of silently ignoring the line.
    EXPECT_THROW(catalog_.loadFromFile(path_), std::runtime_error);
}

TEST_F(TextCatalogTest, UnknownKeyThrows)
{
    // Given a catalog with a key that no TextId declares.
    write(completeCatalog() + "unknown.key=value\n");

    // When it is loaded.
    // Then the failure is explicit.
    EXPECT_THROW(catalog_.loadFromFile(path_), std::runtime_error);
}

TEST_F(TextCatalogTest, DuplicateKeyThrows)
{
    // Given a catalog that declares the same key twice.
    write(completeCatalog() + "menu.title=again\n");

    // When it is loaded.
    // Then the failure is explicit.
    EXPECT_THROW(catalog_.loadFromFile(path_), std::runtime_error);
}

TEST_F(TextCatalogTest, MissingKeyThrows)
{
    // Given a catalog without one declared TextId.
    std::string content = completeCatalog();
    const std::string missing = "menu.quit=menu.quit\n";
    const auto position = content.find(missing);
    ASSERT_NE(position, std::string::npos);
    content.erase(position, missing.size());
    write(content);

    // When it is loaded.
    // Then the failure lists the missing key.
    EXPECT_THROW(catalog_.loadFromFile(path_), std::runtime_error);
}

TEST_F(TextCatalogTest, IgnoresCommentsAndBlankLines)
{
    // Given a valid catalog with comments and blank lines.
    std::string content = completeCatalog();
    const std::string generated = "menu.title=menu.title\n";
    const auto position = content.find(generated);
    ASSERT_NE(position, std::string::npos);
    content.replace(position, generated.size(), "# a comment\n\n   \nmenu.title=WAR\n");
    write(content);

    // When it is loaded.
    catalog_.loadFromFile(path_);

    // Then the comments are ignored and the value is still read.
    EXPECT_EQ(catalog_.text(view::ui::TextId::MenuTitle), "WAR");
}

TEST_F(TextCatalogTest, RealPortugueseCatalogContainsEveryDeclaredKey)
{
    // Given the catalog shipped next to the executable.
    catalog_.load("pt_br");

    // Then every TextId declared in code has a translation.
    for (const auto& entry : view::ui::kTextKeys) {
        EXPECT_TRUE(catalog_.contains(entry.id)) << "missing " << entry.key;
    }
    EXPECT_EQ(catalog_.text(view::ui::TextId::MenuTitle), "WAR");
}
