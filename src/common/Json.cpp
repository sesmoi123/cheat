#include "Json.hpp"

#include <cstdio>
#include <cstdlib>

namespace json {

namespace {

void skip_space(const std::string& text, size_t& index) {
    while (index < text.size() && (text[index] == ' ' || text[index] == '\t' ||
                                   text[index] == '\n' || text[index] == '\r')) {
        ++index;
    }
}

bool parse_string(const std::string& text, size_t& index, std::string& out) {
    if (index >= text.size() || text[index] != '"') {
        return false;
    }
    ++index;

    out.clear();
    while (index < text.size() && text[index] != '"') {
        if (text[index] == '\\' && index + 1 < text.size()) {
            ++index;
            switch (text[index]) {
                case 'n': out.push_back('\n'); break;
                case 't': out.push_back('\t'); break;
                case 'r': out.push_back('\r'); break;
                default: out.push_back(text[index]); break;
            }
        } else {
            out.push_back(text[index]);
        }
        ++index;
    }

    if (index >= text.size()) {
        return false;
    }
    ++index;
    return true;
}

std::string escape(const std::string& value) {
    std::string result;
    result.reserve(value.size() + 2);
    for (char character : value) {
        switch (character) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\t': result += "\\t"; break;
            case '\r': result += "\\r"; break;
            default: result.push_back(character); break;
        }
    }
    return result;
}

} // namespace

bool Value::has(const std::string& key) const {
    return m_entries.find(key) != m_entries.end();
}

double Value::number(const std::string& key, double fallback) const {
    const auto it = m_entries.find(key);
    return it != m_entries.end() && it->second.type == Entry::Type::Number ? it->second.number
                                                                          : fallback;
}

bool Value::boolean(const std::string& key, bool fallback) const {
    const auto it = m_entries.find(key);
    return it != m_entries.end() && it->second.type == Entry::Type::Boolean ? it->second.boolean
                                                                           : fallback;
}

std::string Value::string(const std::string& key, const std::string& fallback) const {
    const auto it = m_entries.find(key);
    return it != m_entries.end() && it->second.type == Entry::Type::String ? it->second.text
                                                                          : fallback;
}

void Value::set_number(const std::string& key, double value) {
    Entry entry;
    entry.type = Entry::Type::Number;
    entry.number = value;
    m_entries[key] = entry;
}

void Value::set_boolean(const std::string& key, bool value) {
    Entry entry;
    entry.type = Entry::Type::Boolean;
    entry.boolean = value;
    m_entries[key] = entry;
}

void Value::set_string(const std::string& key, const std::string& value) {
    Entry entry;
    entry.type = Entry::Type::String;
    entry.text = value;
    m_entries[key] = entry;
}

std::string Value::dump(int indent) const {
    const std::string padding(static_cast<size_t>(indent), ' ');
    std::string result = "{\n";

    size_t index = 0;
    for (const auto& pair : m_entries) {
        const std::string& key = pair.first;
        const Entry& entry = pair.second;

        result += padding + "\"" + escape(key) + "\": ";
        switch (entry.type) {
            case Entry::Type::Number: result += std::to_string(entry.number); break;
            case Entry::Type::Boolean: result += entry.boolean ? "true" : "false"; break;
            case Entry::Type::String: result += "\"" + escape(entry.text) + "\""; break;
        }
        if (++index < m_entries.size()) {
            result += ",";
        }
        result += "\n";
    }

    return result + "}\n";
}

bool Value::parse(const std::string& text, Value& out) {
    size_t index = 0;
    skip_space(text, index);

    if (index >= text.size() || text[index] != '{') {
        return false;
    }
    ++index;

    while (true) {
        skip_space(text, index);
        if (index < text.size() && text[index] == '}') {
            break;
        }

        std::string key;
        if (!parse_string(text, index, key)) {
            return false;
        }

        skip_space(text, index);
        if (index >= text.size() || text[index] != ':') {
            return false;
        }
        ++index;
        skip_space(text, index);

        if (index < text.size() && text[index] == '"') {
            std::string value;
            if (!parse_string(text, index, value)) {
                return false;
            }
            out.set_string(key, value);
        } else if (text.compare(index, 4, "true") == 0) {
            out.set_boolean(key, true);
            index += 4;
        } else if (text.compare(index, 5, "false") == 0) {
            out.set_boolean(key, false);
            index += 5;
        } else if (index < text.size() && text[index] == '{') {
            size_t depth = 0;
            const size_t begin = index;
            do {
                if (text[index] == '{') {
                    ++depth;
                } else if (text[index] == '}') {
                    --depth;
                }
                ++index;
            } while (index < text.size() && depth > 0);
            out.set_string(key, text.substr(begin, index - begin));
        } else {
            char* end = nullptr;
            const double value = std::strtod(text.c_str() + index, &end);
            if (!end || end == text.c_str() + index) {
                return false;
            }
            index = static_cast<size_t>(end - text.c_str());
            out.set_number(key, value);
        }

        skip_space(text, index);
        if (index < text.size() && text[index] == ',') {
            ++index;
        }
    }

    return true;
}

bool read_file(const std::string& path, Value& out) {
    FILE* file = nullptr;
    if (fopen_s(&file, path.c_str(), "rb") != 0 || !file) {
        return false;
    }

    std::string text;
    char buffer[4096];
    size_t read = 0;
    while ((read = std::fread(buffer, 1, sizeof(buffer), file)) > 0) {
        text.append(buffer, read);
    }
    std::fclose(file);

    return Value::parse(text, out);
}

bool write_file(const std::string& path, const Value& value) {
    FILE* file = nullptr;
    if (fopen_s(&file, path.c_str(), "wb") != 0 || !file) {
        return false;
    }
    const std::string text = value.dump();
    std::fwrite(text.data(), 1, text.size(), file);
    std::fclose(file);
    return true;
}

} // namespace json
