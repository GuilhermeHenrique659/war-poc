#include "view/ui/TextCatalog.hpp"

#include "view/ResourcePath.hpp"

#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace view::ui {

namespace {

constexpr std::size_t kTextCount = static_cast<std::size_t>(TextId::Count);

[[nodiscard]] std::size_t indexOf(TextId id)
{
    return static_cast<std::size_t>(id);
}

[[nodiscard]] const std::unordered_map<std::string_view, TextId>& keyIndex()
{
    static const std::unordered_map<std::string_view, TextId> index = [] {
        std::unordered_map<std::string_view, TextId> map;
        map.reserve(kTextKeys.size());
        for (const TextKey& entry : kTextKeys) {
            map.emplace(entry.key, entry.id);
        }
        return map;
    }();
    return index;
}

[[nodiscard]] std::string trim(std::string_view value)
{
    std::size_t begin = 0;
    std::size_t end = value.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(value[begin])) != 0) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }
    return std::string{value.substr(begin, end - begin)};
}

[[nodiscard]] std::string displayPath(const std::filesystem::path& path)
{
    const std::u8string encoded = path.u8string();
    return std::string(encoded.begin(), encoded.end());
}

[[nodiscard]] std::string missingKeys(const std::array<bool, kTextCount>& present)
{
    std::string missing;
    for (const TextKey& entry : kTextKeys) {
        if (present[indexOf(entry.id)]) {
            continue;
        }
        if (!missing.empty()) {
            missing += ", ";
        }
        missing += entry.key;
    }
    return missing;
}

} // namespace

TextCatalog& TextCatalog::instance()
{
    static TextCatalog catalog;
    return catalog;
}

void TextCatalog::load(std::string_view language)
{
    loadFromFile(view::resourcePath("texts/" + std::string(language) + ".txt"));
}

void TextCatalog::loadFromFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Unable to open text catalog: " + displayPath(path));
    }

    std::array<std::string, kTextCount> texts{};
    std::array<bool, kTextCount> present{};
    std::string line;
    std::size_t lineNumber = 0;

    while (std::getline(file, line)) {
        ++lineNumber;
        if (lineNumber == 1 && line.size() >= 3 && line.compare(0, 3, "\xEF\xBB\xBF") == 0) {
            line.erase(0, 3);
        }

        const std::string entry = trim(line);
        if (entry.empty() || entry.front() == '#') {
            continue;
        }

        const std::size_t separator = entry.find('=');
        if (separator == std::string::npos) {
            throw std::runtime_error(
                "Malformed text catalog line " + std::to_string(lineNumber) + " in " + displayPath(path));
        }

        const std::string key = trim(std::string_view{entry}.substr(0, separator));
        const std::string value = trim(std::string_view{entry}.substr(separator + 1));
        if (key.empty()) {
            throw std::runtime_error(
                "Empty text catalog key on line " + std::to_string(lineNumber) + " in " + displayPath(path));
        }

        const auto found = keyIndex().find(key);
        if (found == keyIndex().end()) {
            throw std::runtime_error(
                "Unknown text catalog key '" + key + "' on line " + std::to_string(lineNumber) + " in "
                + displayPath(path));
        }

        const std::size_t index = indexOf(found->second);
        if (present[index]) {
            throw std::runtime_error(
                "Duplicate text catalog key '" + key + "' on line " + std::to_string(lineNumber) + " in "
                + displayPath(path));
        }

        texts[index] = value;
        present[index] = true;
    }

    if (file.bad()) {
        throw std::runtime_error("Failed while reading text catalog: " + displayPath(path));
    }

    const std::string missing = missingKeys(present);
    if (!missing.empty()) {
        throw std::runtime_error(
            "Text catalog " + displayPath(path) + " is missing keys: " + missing);
    }

    texts_ = std::move(texts);
    present_ = present;
}

const std::string& TextCatalog::text(TextId id) const
{
    const std::size_t index = indexOf(id);
    if (!present_[index]) {
        throw std::runtime_error("Text not loaded: " + std::string(textKey(id)));
    }
    return texts_[index];
}

bool TextCatalog::contains(TextId id) const
{
    return present_[indexOf(id)];
}

void TextCatalog::clear()
{
    texts_ = {};
    present_ = {};
}

const std::string& text(TextId id)
{
    return TextCatalog::instance().text(id);
}

} // namespace view::ui
