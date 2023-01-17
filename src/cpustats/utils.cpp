#include "utils.hpp"
#include <cctype>

const char *ToNextWord(const char *s, int cnt) {
    while (cnt > 0) {
        // Find word end
        while (*s && !std::isspace(*s)) {
            s++;
        }
        // Find next word start
        while (*s && std::isspace(*s)) {
            s++;
        }
        cnt--;
    }
    return s;
}

const char *ToWordEnd(const char *s) {
    if (!s) return s;
    while (*s && !std::isspace(*s)) {
        s++;
    }
    return s;
}
