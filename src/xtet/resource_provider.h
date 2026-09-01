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

bool load_sfs_file(const char *sfs_name, std::vector<uint8_t> &bytes, std::string &error);

} // namespace xtet
