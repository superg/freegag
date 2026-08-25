#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string_view>
#include <vector>
#include "resource_provider.h"

namespace xtet
{

struct SfsHeader
{
    uint32_t version{};
    uint32_t checksum{};
    uint32_t entry_count{};
    uint32_t directory_offset{};
    uint32_t allocation_map_offset{};
    uint32_t data_offset{};
    uint32_t virtual_size{};
    uint32_t maximum_path_length{};
};

struct SfsEntry
{
    uint32_t hash_a{};
    uint32_t hash_b{};
    uint32_t attributes{};
    uint32_t virtual_offset{};
    uint32_t size{};
    uint32_t stored14{};
    uint32_t flags{};
    uint32_t stored1c{};
};

class SfsArchive
{
public:
    bool mount(ResourceView resource);
    bool valid() const;
    const SfsEntry *find(std::string_view path) const;
    bool read(const SfsEntry &entry, std::vector<uint8_t> &bytes) const;
    bool read(std::string_view path, std::vector<uint8_t> &bytes) const;

private:
    bool decode_block(size_t block_index, std::vector<uint8_t> &bytes) const;

    ResourceView resource_{};
    SfsHeader header_{};
    std::vector<SfsEntry> entries_;
    std::vector<uint32_t> allocation_map_;
};

} // namespace xtet
