#pragma once

#include <algorithm>
#include <filesystem>
#include <string>
#include <utility>
#include "portable_string.h"

namespace freegag
{
inline std::filesystem::path normalize_host_path(const std::filesystem::path &path)
{
    std::string text = path.string();
    std::replace(text.begin(), text.end(), '\\', static_cast<char>(std::filesystem::path::preferred_separator));
    return std::filesystem::path(text);
}

inline bool resolve_existing_host_path_case_insensitive(const std::filesystem::path &requested_path, std::filesystem::path *resolved_path)
{
    if(resolved_path == nullptr || requested_path.empty())
        return false;

    const std::filesystem::path normalized_path = normalize_host_path(requested_path);
    std::error_code error;
    if(std::filesystem::exists(normalized_path, error))
    {
        *resolved_path = normalized_path;
        return true;
    }

    std::filesystem::path current = normalized_path.root_path();
    if(current.empty())
        current = ".";
    for(const std::filesystem::path &component : normalized_path.relative_path())
    {
        if(component == ".")
            continue;
        if(component == "..")
        {
            current /= component;
            continue;
        }

        const std::filesystem::path exact_path = current / component;
        error.clear();
        if(std::filesystem::exists(exact_path, error))
        {
            current = exact_path;
            continue;
        }

        std::filesystem::path matched_path;
        error.clear();
        for(std::filesystem::directory_iterator entry(current, error), end; !error && entry != end; entry.increment(error))
        {
            const std::string entry_name = entry->path().filename().string();
            const std::string component_name = component.string();
            if(compare_ascii_case_insensitive(entry_name.c_str(), component_name.c_str()) == 0 && (matched_path.empty() || entry_name < matched_path.filename().string()))
                matched_path = entry->path();
        }
        if(error || matched_path.empty())
            return false;
        current = std::move(matched_path);
    }

    if(!normalized_path.is_absolute())
    {
        const std::filesystem::path relative_path = current.lexically_relative(".");
        if(!relative_path.empty())
            current = relative_path;
    }
    *resolved_path = current;
    return true;
}
} // namespace freegag
