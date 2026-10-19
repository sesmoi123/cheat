#pragma once

#include <map>
#include <string>

namespace json {

// Mini parseur JSON plat : { "cle": nombre|"chaine"|true|false, ... }.
// Suffisant pour config.json et offsets.json. Les objets imbriques sont
// conserves sous forme de texte brut.
class Value {
public:
    bool has(const std::string& key) const;

    double number(const std::string& key, double fallback = 0.0) const;
    bool boolean(const std::string& key, bool fallback = false) const;
    std::string string(const std::string& key, const std::string& fallback = {}) const;

    void set_number(const std::string& key, double value);
    void set_boolean(const std::string& key, bool value);
    void set_string(const std::string& key, const std::string& value);

    std::string dump(int indent = 2) const;

    static bool parse(const std::string& text, Value& out);

private:
    struct Entry {
        enum class Type { Number, Boolean, String };
        Type type = Type::Number;
        double number = 0.0;
        bool boolean = false;
        std::string text;
    };

    std::map<std::string, Entry> m_entries;
};

bool read_file(const std::string& path, Value& out);
bool write_file(const std::string& path, const Value& value);

} // namespace json
