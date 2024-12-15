#include "str_split.h"

void str_split(std::vector<String>& vector, const char* buf, size_t len, const char delimiter) {
    String sub;
    for (size_t i = 0; i < len; i++) {
        if (buf[i] == delimiter) {
            vector.push_back(sub);
            sub.clear();
        } else
            sub += buf[i];
    }
    vector.push_back(sub);
}

void str_split(std::vector<String>& vector, const String& str, const char delimiter) {
    str_split(vector, str.c_str(), str.length(), delimiter);
}

std::vector<String> str_split(const char* buf, size_t len, const char delimiter) {
    std::vector<String> vector;
    str_split(vector, buf, len, delimiter);
    return vector;
}

std::vector<String> str_split(const String& str, const char delimiter) {
    std::vector<String> vector;
    str_split(vector, str.c_str(), str.length(), delimiter);
    return vector;
}
