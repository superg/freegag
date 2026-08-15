#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>
#include "resource_provider.h"

namespace xtet
{

struct SfsHeader
{
    std::uint32_t version{};
    std::uint32_t checksum{};
    std::uint32_t entry_count{};
    std::uint32_t directory_offset{};
    std::uint32_t allocation_map_offset{};
    std::uint32_t data_offset{};
    std::uint32_t virtual_size{};
    std::uint32_t maximum_path_length{};
};

struct SfsEntry
{
    std::uint32_t hash_a{};
    std::uint32_t hash_b{};
    std::uint32_t attributes{};
    std::uint32_t virtual_offset{};
    std::uint32_t size{};
    std::uint32_t stored14{};
    std::uint32_t flags{};
    std::uint32_t stored1c{};
};

class SfsArchive
{
public:
    bool mount(ResourceView resource);
    bool valid() const;
    const SfsHeader &header() const;
    const SfsEntry *find(std::string_view path) const;
    bool read(const SfsEntry &entry, std::vector<std::uint8_t> &bytes) const;
    bool read(std::string_view path, std::vector<std::uint8_t> &bytes) const;

private:
    bool decode_block(std::size_t block_index, std::vector<std::uint8_t> &bytes) const;

    ResourceView resource_{};
    SfsHeader header_{};
    std::vector<SfsEntry> entries_;
    std::vector<std::uint32_t> allocation_map_;
};

} // namespace xtet
