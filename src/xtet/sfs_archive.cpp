#include "sfs_archive.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <zlib.h>

namespace xtet
{
namespace
{

constexpr size_t kHeaderSize = 0x100;
constexpr size_t kDirectoryEntrySize = 0x20;
constexpr uint32_t kVersion = 200;
constexpr uint32_t kAllocationBlockSize = 0x8000;
constexpr std::array<uint32_t, 8> kHashBasisA{ 0x23788d5e, 0x46f11abc, 0x0de23578, 0x38bce7ae, 0x7179cf5c, 0x62f39eb8, 0x669fb02e, 0x6e47ed02 };
constexpr std::array<uint32_t, 8> kHashBasisB{ 0x1434182e, 0x3c5c2872, 0x6c8c48ca, 0x4d2c89ba, 0x1a591374, 0x20863ec6, 0x553865a2, 0x3e44d36a };

uint32_t read_u32(const ResourceView &resource, size_t offset)
{
    uint32_t value{};
    std::memcpy(&value, resource.data + offset, sizeof(value));
    return value;
}

bool contains(const ResourceView &resource, uint64_t offset, uint64_t size)
{
    return offset <= resource.size && size <= resource.size - offset;
}

uint32_t hash_table_value(uint8_t index, const std::array<uint32_t, 8> &basis)
{
    uint32_t value = 0;
    for(size_t bit = 0; bit < basis.size(); ++bit)
    {
        if((index & (1u << bit)) != 0)
            value ^= basis[bit];
    }
    return value;
}

std::array<uint32_t, 2> hash_path(std::string_view path)
{
    std::array<uint32_t, 2> hash{};
    for(unsigned char character : path)
    {
        const uint8_t upper = (uint8_t)std::toupper(character);
        hash[0] = (hash[0] << 8 | upper) ^ hash_table_value((uint8_t)(hash[0] >> 24), kHashBasisA);
        hash[1] = (hash[1] << 8 | upper) ^ hash_table_value((uint8_t)(hash[1] >> 24), kHashBasisB);
    }
    return hash;
}

bool decode_lzss(const uint8_t *source, size_t source_size, std::vector<uint8_t> &output)
{
    std::array<uint8_t, 4096> window{};
    size_t window_position = 0xfee;
    size_t source_position = 0;
    uint32_t control = 0;
    output.clear();
    output.reserve(kAllocationBlockSize);
    while(source_position < source_size)
    {
        control >>= 1;
        if((control & 0x100) == 0)
            control = source[source_position++] | 0xff00;
        if((control & 1) != 0)
        {
            if(source_position >= source_size)
                return true;
            if(output.size() >= kAllocationBlockSize)
                return false;
            const uint8_t value = source[source_position++];
            output.push_back(value);
            window[window_position] = value;
            window_position = (window_position + 1) & 0xfff;
        }
        else
        {
            if(source_size - source_position < 2)
                return true;
            size_t copy_position = source[source_position] | (size_t)(source[source_position + 1] & 0xf0) << 4;
            const size_t copy_size = (source[source_position + 1] & 0x0f) + 3;
            source_position += 2;
            if(copy_size > kAllocationBlockSize - output.size())
                return false;
            for(size_t index = 0; index < copy_size; ++index)
            {
                const uint8_t value = window[copy_position];
                copy_position = (copy_position + 1) & 0xfff;
                output.push_back(value);
                window[window_position] = value;
                window_position = (window_position + 1) & 0xfff;
            }
        }
    }
    return true;
}

} // namespace

bool SfsArchive::mount(ResourceView resource)
{
    resource_ = {};
    header_ = {};
    entries_.clear();
    allocation_map_.clear();
    if(!resource.data || resource.size < kHeaderSize || std::memcmp(resource.data, "SFS\0", 4) != 0)
        return false;

    SfsHeader header;
    header.version = read_u32(resource, 0x04);
    header.checksum = read_u32(resource, 0x08);
    header.entry_count = read_u32(resource, 0x0c);
    header.directory_offset = read_u32(resource, 0x10);
    header.allocation_map_offset = read_u32(resource, 0x14);
    header.data_offset = read_u32(resource, 0x18);
    header.virtual_size = read_u32(resource, 0x1c);
    header.maximum_path_length = read_u32(resource, 0x20);
    if(header.version != kVersion)
        return false;

    uint32_t checksum = 0;
    for(size_t offset = 0; offset < kHeaderSize; ++offset)
    {
        if(offset < 0x08 || offset >= 0x0c)
            checksum += resource.data[offset];
    }
    if(checksum != header.checksum)
        return false;

    const uint64_t directory_size = (uint64_t)header.entry_count * kDirectoryEntrySize;
    const uint64_t allocation_word_count = ((uint64_t)header.virtual_size + kAllocationBlockSize - 1) / kAllocationBlockSize + 1;
    const uint64_t allocation_map_size = allocation_word_count * sizeof(uint32_t);
    if(header.directory_offset < kHeaderSize || !contains(resource, header.directory_offset, directory_size) || !contains(resource, header.allocation_map_offset, allocation_map_size)
        || header.allocation_map_offset < (uint64_t)header.directory_offset + directory_size || header.data_offset < (uint64_t)header.allocation_map_offset + allocation_map_size
        || header.data_offset > resource.size || header.maximum_path_length == 0)
        return false;

    entries_.reserve(header.entry_count);
    for(uint32_t index = 0; index < header.entry_count; ++index)
    {
        const size_t offset = header.directory_offset + (size_t)index * kDirectoryEntrySize;
        entries_.push_back({ read_u32(resource, offset), read_u32(resource, offset + 4), read_u32(resource, offset + 8), read_u32(resource, offset + 12), read_u32(resource, offset + 16),
            read_u32(resource, offset + 20), read_u32(resource, offset + 24), read_u32(resource, offset + 28) });
    }
    allocation_map_.reserve((size_t)allocation_word_count);
    for(size_t index = 0; index < allocation_word_count; ++index)
        allocation_map_.push_back(read_u32(resource, header.allocation_map_offset + index * sizeof(uint32_t)));
    if(!std::is_sorted(entries_.begin(), entries_.end(),
           [](const SfsEntry &left, const SfsEntry &right) { return left.hash_a < right.hash_a || (left.hash_a == right.hash_a && left.hash_b < right.hash_b); })
        || !std::is_sorted(allocation_map_.begin(), allocation_map_.end()) || allocation_map_.back() > resource.size)
    {
        entries_.clear();
        allocation_map_.clear();
        return false;
    }

    resource_ = resource;
    header_ = header;
    return true;
}

bool SfsArchive::valid() const
{
    return resource_.data != nullptr;
}

const SfsHeader &SfsArchive::header() const
{
    return header_;
}

const SfsEntry *SfsArchive::find(std::string_view path) const
{
    if(!valid() || path.empty() || path.size() > header_.maximum_path_length)
        return nullptr;
    const std::array<uint32_t, 2> hash = hash_path(path);
    const auto iterator = std::lower_bound(entries_.begin(), entries_.end(), hash,
        [](const SfsEntry &entry, const std::array<uint32_t, 2> &value) { return entry.hash_a < value[0] || (entry.hash_a == value[0] && entry.hash_b < value[1]); });
    return iterator != entries_.end() && iterator->hash_a == hash[0] && iterator->hash_b == hash[1] ? &*iterator : nullptr;
}

bool SfsArchive::decode_block(size_t block_index, std::vector<uint8_t> &bytes) const
{
    if(block_index + 1 >= allocation_map_.size())
        return false;
    const uint32_t begin = allocation_map_[block_index];
    const uint32_t end = allocation_map_[block_index + 1];
    if(end < begin || !contains(resource_, begin, end - begin) || end == begin || end - begin > kAllocationBlockSize)
        return false;
    const uint8_t *source = resource_.data + begin;
    const size_t source_size = end - begin;
    if(source_size == kAllocationBlockSize)
    {
        bytes.assign(source, source + source_size);
        return true;
    }
    if(source_size < 2)
        return false;
    if(source[0] == 1)
        return decode_lzss(source + 1, source_size - 1, bytes);
    if(source_size < 3)
        return false;
    const uint16_t encoding = (uint16_t)(source[1] | source[2] << 8);
    if(encoding == 0)
    {
        bytes.assign(source + 3, source + source_size);
        return bytes.size() <= kAllocationBlockSize;
    }
    if(encoding != 8)
        return false;
    bytes.resize(kAllocationBlockSize);
    z_stream stream{};
    stream.next_in = (Bytef *)source + 3;
    stream.avail_in = (uInt)source_size - 3;
    stream.next_out = bytes.data();
    stream.avail_out = (uInt)bytes.size();
    if(inflateInit2(&stream, -MAX_WBITS) != Z_OK)
        return false;
    const int result = inflate(&stream, Z_FINISH);
    const size_t decoded_size = stream.total_out;
    inflateEnd(&stream);
    // Some full SFS blocks end exactly at the 32 KiB output boundary before the inflater can consume an end marker. Accept that complete logical block; partial output still requires a clean end.
    if(result != Z_STREAM_END && decoded_size != kAllocationBlockSize)
        return false;
    bytes.resize(decoded_size);
    return true;
}

bool SfsArchive::read(const SfsEntry &entry, std::vector<uint8_t> &bytes) const
{
    bytes.clear();
    if(!valid() || entry.virtual_offset > header_.virtual_size || entry.size > header_.virtual_size - entry.virtual_offset)
        return false;
    bytes.reserve(entry.size);
    uint64_t position = entry.virtual_offset;
    uint32_t remaining = entry.size;
    std::vector<uint8_t> block;
    while(remaining != 0)
    {
        const size_t block_index = (size_t)(position / kAllocationBlockSize);
        const size_t block_offset = (size_t)(position % kAllocationBlockSize);
        if(!decode_block(block_index, block) || block_offset >= block.size())
            return false;
        const size_t copy_size = std::min<size_t>(remaining, block.size() - block_offset);
        bytes.insert(bytes.end(), block.begin() + (ptrdiff_t)block_offset, block.begin() + (ptrdiff_t)(block_offset + copy_size));
        position += copy_size;
        remaining -= (uint32_t)copy_size;
    }
    return true;
}

bool SfsArchive::read(std::string_view path, std::vector<uint8_t> &bytes) const
{
    const SfsEntry *entry = find(path);
    return entry && read(*entry, bytes);
}

} // namespace xtet
