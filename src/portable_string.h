#pragma once

#include <cctype>
#include <cstddef>

namespace freegag
{
inline int compare_ascii_case_insensitive(const char *left, const char *right)
{
    if(left == nullptr || right == nullptr)
        return left == right ? 0 : (left == nullptr ? -1 : 1);
    while(*left != '\0' && *right != '\0')
    {
        const int left_character = std::tolower(static_cast<unsigned char>(*left));
        const int right_character = std::tolower(static_cast<unsigned char>(*right));
        if(left_character != right_character)
            return left_character < right_character ? -1 : 1;
        ++left;
        ++right;
    }
    if(*left == *right)
        return 0;
    return *left == '\0' ? -1 : 1;
}

inline int compare_ascii_case_insensitive(const char *left, const char *right, size_t count)
{
    for(size_t index = 0; index < count; ++index)
    {
        const unsigned char left_character = static_cast<unsigned char>(left[index]);
        const unsigned char right_character = static_cast<unsigned char>(right[index]);
        const int left_folded = std::tolower(left_character);
        const int right_folded = std::tolower(right_character);
        if(left_folded != right_folded)
            return left_folded < right_folded ? -1 : 1;
        if(left_character == '\0')
            return 0;
    }
    return 0;
}
} // namespace freegag
