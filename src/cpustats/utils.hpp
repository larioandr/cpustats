#ifndef CPUSTATS_UTILS_HPP
#define CPUSTATS_UTILS_HPP

#include <array>
#include <string>

/**
 * Find the beginning of the next word.
 *
 * Assuming s points some non-space character,
 * skip all non-space chars, then all space chars
 * and return a pointer to the first non-space char
 * afterwards, or end of string ('\0').
 *
 * @param s c-string
 * @param cnt number of words to skip
 * @return pointer to the start of the next word
 */
const char *ToNextWord(const char *s, int cnt = 1);

/**
 * Find the first char after current word.
 * If s points to a whitespace char, then return itself.
 *
 * @param s
 * @return
 */
const char *ToWordEnd(const char *s);

template<typename T, size_t k>
void Subtract(std::array<T, k> const& curr, std::array<T, k> const& prev, std::array<T, k>& result) {
    for (size_t i = 0; i < k; i++) {
        result[i] = curr[i] - prev[i];
    }
}

#endif //CPUSTATS_UTILS_HPP
