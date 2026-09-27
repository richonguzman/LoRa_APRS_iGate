#pragma once

#include <cstdint>
#include <cstdlib>
#include <string>

class String {
public:
    std::string value;

    String() = default;
    String(const char* text) : value(text ? text : "") {}
    String(const std::string& text) : value(text) {}
    String(char character) : value(1, character) {}
    String(int number) : value(std::to_string(number)) {}

    unsigned int length() const { return static_cast<unsigned int>(value.size()); }
    const char* c_str() const { return value.c_str(); }
    bool isEmpty() const { return value.empty(); }

    int indexOf(const String& text) const {
        auto index = value.find(text.value);
        return index == std::string::npos ? -1 : static_cast<int>(index);
    }

    int indexOf(const String& text, unsigned int from) const {
        if (from > value.size()) return -1;
        auto index = value.find(text.value, from);
        return index == std::string::npos ? -1 : static_cast<int>(index);
    }

    int indexOf(char character) const {
        auto index = value.find(character);
        return index == std::string::npos ? -1 : static_cast<int>(index);
    }

    int indexOf(char character, unsigned int from) const {
        if (from > value.size()) return -1;
        auto index = value.find(character, from);
        return index == std::string::npos ? -1 : static_cast<int>(index);
    }

    bool startsWith(const String& prefix) const {
        return value.size() >= prefix.value.size()
            && value.compare(0, prefix.value.size(), prefix.value) == 0;
    }

    long toInt() const {
        char* end = nullptr;
        long result = std::strtol(value.c_str(), &end, 10);
        return end == value.c_str() ? 0 : result;
    }

    String substring(unsigned int from) const {
        if (from >= value.size()) return String();
        return String(value.substr(from));
    }

    String substring(unsigned int from, unsigned int to) const {
        if (from >= value.size() || to <= from) return String();
        if (to > value.size()) to = static_cast<unsigned int>(value.size());
        return String(value.substr(from, to - from));
    }

    void remove(unsigned int index, unsigned int count) {
        if (index < value.size()) value.erase(index, count);
    }

    void replace(const String& find, const String& replacement) {
        if (value.empty() || find.value.empty()) return;
        std::size_t position = 0;
        while ((position = value.find(find.value, position)) != std::string::npos) {
            value.replace(position, find.value.size(), replacement.value);
            position += replacement.value.size();
        }
    }

    char operator[](unsigned int index) const {
        return index < value.size() ? value[index] : '\0';
    }

    bool endsWith(const String& suffix) const {
        return value.size() >= suffix.value.size()
            && value.compare(value.size() - suffix.value.size(),
                             suffix.value.size(), suffix.value) == 0;
    }

    void trim() {
        constexpr const char* whitespace = " \t\r\n";
        auto begin = value.find_first_not_of(whitespace);
        if (begin == std::string::npos) {
            value.clear();
            return;
        }
        auto end = value.find_last_not_of(whitespace);
        value = value.substr(begin, end - begin + 1);
    }

    void toUpperCase() {
        for (char& character : value) {
            if (character >= 'a' && character <= 'z') character -= ('a' - 'A');
        }
    }

    String& operator+=(const String& other) {
        value += other.value;
        return *this;
    }

    bool operator==(const String& other) const { return value == other.value; }
    bool operator!=(const String& other) const { return value != other.value; }
};

inline String operator+(const String& left, const String& right) {
    String result(left);
    result += right;
    return result;
}

inline String operator+(const String& left, const char* right) {
    return left + String(right);
}

inline String operator+(const char* left, const String& right) {
    return String(left) + right;
}

inline unsigned long millis() { return 0; }
