#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace xtet
{

struct ResourceView
{
    const uint8_t *data{};
    size_t size{};
};

bool load_sfs_from_working_directory(const char *sfs_name, std::vector<uint8_t> &bytes, std::string &error);

} // namespace xtet
