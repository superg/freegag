#include "sfs_archive.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <limits>
#include <numeric>
#include <string>
#include "legacy_gzip_codec.h"

namespace xtet
{

constexpr size_t kHeaderSize = 0x100;
constexpr size_t kDirectoryEntrySize = 0x20;
constexpr size_t kCatalogEntrySize = 0x200;
constexpr size_t kCatalogAreaSize = 0x10000;
constexpr size_t kHeaderCommentOffset = 0x22;
constexpr size_t kHeaderCommentCapacity = kHeaderSize - kHeaderCommentOffset;
constexpr uint32_t kVersion = 200;
constexpr uint32_t kAllocationBlockSize = 0x8000;
constexpr uint32_t kDirectoryFlag = 0x10;
constexpr uint32_t kFileFlag = 0x20;
constexpr std::array<uint32_t, 8> kHashBasisA{ 0x23788d5e, 0x46f11abc, 0x0de23578, 0x38bce7ae, 0x7179cf5c, 0x62f39eb8, 0x669fb02e, 0x6e47ed02 };
constexpr std::array<uint32_t, 8> kHashBasisB{ 0x1434182e, 0x3c5c2872, 0x6c8c48ca, 0x4d2c89ba, 0x1a591374, 0x20863ec6, 0x553865a2, 0x3e44d36a };

uint32_t read_u32(const ResourceView &resource, size_t offset)
{
    uint32_t value{};
    std::memcpy(&value, resource.data + offset, sizeof(value));
    return value;
}

uint16_t read_u16(const ResourceView &resource, size_t offset)
{
    uint16_t value{};
    std::memcpy(&value, resource.data + offset, sizeof(value));
    return value;
}

uint64_t read_u64(const uint8_t *source)
{
    uint64_t value{};
    std::memcpy(&value, source, sizeof(value));
    return value;
}

void write_u32(std::vector<uint8_t> &bytes, size_t offset, uint32_t value)
{
    std::memcpy(bytes.data() + offset, &value, sizeof(value));
}

void write_u16(std::vector<uint8_t> &bytes, size_t offset, uint16_t value)
{
    std::memcpy(bytes.data() + offset, &value, sizeof(value));
}

void write_u64(std::vector<uint8_t> &bytes, size_t offset, uint64_t value)
{
    std::memcpy(bytes.data() + offset, &value, sizeof(value));
}

bool contains(const ResourceView &resource, uint64_t offset, uint64_t size)
{
    return offset <= resource.size && size <= resource.size - offset;
}

uint32_t hash_table_value(uint8_t index, const std::array<uint32_t, 8> &basis)
{
    uint32_t value = 0;
    for(size_t bit = 0; bit < basis.size(); ++bit)
        if((index & (1u << bit)) != 0)
            value ^= basis[bit];
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

bool read_padded_name(const uint8_t *source, size_t capacity, std::string &name)
{
    const uint8_t *terminator = std::find(source, source + capacity, 0);
    if(terminator == source + capacity || std::any_of(terminator + 1, source + capacity, [](uint8_t value) { return value != 0; }))
        return false;
    name.assign(reinterpret_cast<const char *>(source), static_cast<size_t>(terminator - source));
    return !name.empty();
}

bool read_padded_string(const uint8_t *source, size_t capacity, std::string &value)
{
    const uint8_t *terminator = std::find(source, source + capacity, 0);
    if(terminator == source + capacity || std::any_of(terminator + 1, source + capacity, [](uint8_t byte) { return byte != 0; }))
        return false;
    value.assign(reinterpret_cast<const char *>(source), static_cast<size_t>(terminator - source));
    return true;
}

bool hash_less(const SfsEntry &left, const SfsEntry &right)
{
    return left.hash_a < right.hash_a || (left.hash_a == right.hash_a && left.hash_b < right.hash_b);
}

bool is_marker(std::string_view name)
{
    return name == "." || name == "...";
}

uint32_t byte_sum(const std::vector<uint8_t> &bytes)
{
    return std::accumulate(bytes.begin(), bytes.end(), uint32_t{});
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
    header.maximum_path_length = read_u16(resource, 0x20);
    if(header.version != kVersion)
        return false;

    uint32_t checksum = 0;
    for(size_t offset = 0; offset < kHeaderSize; ++offset)
        if(offset < 0x08 || offset >= 0x0c)
            checksum += resource.data[offset];
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
    if(!std::is_sorted(entries_.begin(), entries_.end(), hash_less) || !std::is_sorted(allocation_map_.begin(), allocation_map_.end()) || allocation_map_.back() > resource.size)
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
    const int decoded_size = freegag::legacy_gzip_decompress(source + 3, static_cast<uint32_t>(source_size - 3), bytes.data(), static_cast<uint32_t>(bytes.size()), true);
    if(decoded_size < 0)
        return false;
    bytes.resize(static_cast<size_t>(decoded_size));
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

bool SfsArchive::read_catalog(std::vector<SfsCatalogEntry> &catalog, SfsArchiveMetadata *metadata) const
{
    catalog.clear();
    if(!valid() || header_.entry_count < 2 || header_.entry_count > kCatalogAreaSize / kCatalogEntrySize || header_.directory_offset != kHeaderSize
        || header_.allocation_map_offset != header_.directory_offset + header_.entry_count * kDirectoryEntrySize
        || header_.data_offset != header_.allocation_map_offset + allocation_map_.size() * sizeof(uint32_t) || allocation_map_.front() != header_.data_offset
        || allocation_map_.back() != resource_.size || header_.virtual_size < kCatalogAreaSize)
        return false;

    SfsArchiveMetadata decoded_metadata;
    if(!read_padded_string(resource_.data + kHeaderCommentOffset, kHeaderCommentCapacity, decoded_metadata.header_comment))
        return false;

    std::vector<uint8_t> virtual_data;
    virtual_data.reserve((allocation_map_.size() - 1) * kAllocationBlockSize);
    std::vector<uint8_t> block;
    for(size_t index = 0; index + 1 < allocation_map_.size(); ++index)
    {
        if(!decode_block(index, block) || block.size() != kAllocationBlockSize)
            return false;
        virtual_data.insert(virtual_data.end(), block.begin(), block.end());
    }
    if(header_.virtual_size > virtual_data.size() || std::any_of(virtual_data.begin() + header_.virtual_size, virtual_data.end(), [](uint8_t value) { return value != 0; })
        || std::any_of(virtual_data.begin() + header_.entry_count * kCatalogEntrySize, virtual_data.begin() + kCatalogAreaSize, [](uint8_t value) { return value != 0; }))
        return false;

    std::vector<const SfsEntry *> entries_by_ordinal(header_.entry_count);
    std::vector<size_t> directory_positions(header_.entry_count);
    for(size_t directory_position = 0; directory_position < entries_.size(); ++directory_position)
    {
        const SfsEntry &entry = entries_[directory_position];
        if(entry.attributes >= entries_by_ordinal.size() || entries_by_ordinal[entry.attributes] != nullptr)
            return false;
        entries_by_ordinal[entry.attributes] = &entry;
        directory_positions[entry.attributes] = directory_position;
        if(directory_position != 0 && !hash_less(entries_[directory_position - 1], entry))
            return false;
    }

    catalog.resize(header_.entry_count);
    uint32_t maximum_path_length = 0;
    for(size_t ordinal = 0; ordinal < catalog.size(); ++ordinal)
    {
        const SfsEntry &entry = *entries_by_ordinal[ordinal];
        const size_t metadata_offset = ordinal * kCatalogEntrySize;
        std::string first_name;
        std::string second_name;
        if(read_u32({ virtual_data.data(), virtual_data.size() }, metadata_offset) != directory_positions[ordinal] || !read_padded_name(virtual_data.data() + metadata_offset + 4, 0xfc, first_name)
            || read_u32({ virtual_data.data(), virtual_data.size() }, metadata_offset + 0x100) != 0 || !read_padded_name(virtual_data.data() + metadata_offset + 0x11c, 0xe4, second_name)
            || first_name != second_name)
            return false;
        const std::array<uint32_t, 2> hash = hash_path(first_name);
        if(entry.hash_a != hash[0] || entry.hash_b != hash[1] || entry.stored14 != 2)
            return false;

        const bool marker = is_marker(first_name);
        if((marker && entry.flags != kDirectoryFlag) || (!marker && entry.flags != kFileFlag) || (ordinal == 0) != (first_name == ".") || (ordinal + 1 == catalog.size()) != (first_name == "...")
            || (marker && (entry.virtual_offset != kCatalogAreaSize || entry.size != 0 || entry.stored1c != 0)))
            return false;

        SfsCatalogEntry &catalog_entry = catalog[ordinal];
        catalog_entry.name = std::move(first_name);
        catalog_entry.creation_time = read_u64(virtual_data.data() + metadata_offset + 0x104);
        catalog_entry.access_time = read_u64(virtual_data.data() + metadata_offset + 0x10c);
        catalog_entry.write_time = read_u64(virtual_data.data() + metadata_offset + 0x114);
        maximum_path_length = std::max(maximum_path_length, static_cast<uint32_t>(catalog_entry.name.size()));
    }

    uint64_t virtual_position = kCatalogAreaSize;
    for(size_t ordinal = catalog.size() - 1; ordinal-- > 1;)
    {
        const SfsEntry &entry = *entries_by_ordinal[ordinal];
        if(entry.virtual_offset != virtual_position || virtual_position > header_.virtual_size || entry.size > header_.virtual_size - virtual_position)
            return false;
        SfsCatalogEntry &catalog_entry = catalog[ordinal];
        catalog_entry.content.assign(virtual_data.begin() + entry.virtual_offset, virtual_data.begin() + entry.virtual_offset + entry.size);
        if(entry.stored1c != byte_sum(catalog_entry.content))
            return false;
        virtual_position += entry.size;
    }
    if(virtual_position != header_.virtual_size || maximum_path_length != header_.maximum_path_length)
        return false;
    if(metadata != nullptr)
        *metadata = std::move(decoded_metadata);
    return true;
}

bool build_sfs_archive(const std::vector<SfsCatalogEntry> &catalog, std::vector<uint8_t> &bytes)
{
    return build_sfs_archive(catalog, {}, bytes);
}

bool build_sfs_archive(const std::vector<SfsCatalogEntry> &catalog, const SfsArchiveMetadata &metadata, std::vector<uint8_t> &bytes)
{
    bytes.clear();
    if(catalog.size() < 2 || catalog.size() > kCatalogAreaSize / kCatalogEntrySize || catalog.front().name != "." || catalog.back().name != "..."
        || metadata.header_comment.size() >= kHeaderCommentCapacity || metadata.header_comment.find('\0') != std::string::npos)
        return false;

    std::vector<SfsEntry> entries(catalog.size());
    uint64_t virtual_size = kCatalogAreaSize;
    uint32_t maximum_path_length = 0;
    for(size_t ordinal = 0; ordinal < catalog.size(); ++ordinal)
    {
        const SfsCatalogEntry &catalog_entry = catalog[ordinal];
        const bool marker = is_marker(catalog_entry.name);
        if(catalog_entry.name.empty() || catalog_entry.name.size() > 0xe3 || catalog_entry.name.find('\0') != std::string::npos || marker != (ordinal == 0 || ordinal + 1 == catalog.size())
            || (marker && !catalog_entry.content.empty()) || catalog_entry.content.size() > std::numeric_limits<uint32_t>::max())
            return false;
        maximum_path_length = std::max(maximum_path_length, static_cast<uint32_t>(catalog_entry.name.size()));
        if(!marker)
        {
            virtual_size += catalog_entry.content.size();
            if(virtual_size > std::numeric_limits<uint32_t>::max())
                return false;
        }
        const std::array<uint32_t, 2> hash = hash_path(catalog_entry.name);
        entries[ordinal] = { hash[0], hash[1], static_cast<uint32_t>(ordinal), kCatalogAreaSize, static_cast<uint32_t>(catalog_entry.content.size()), 2, marker ? kDirectoryFlag : kFileFlag,
            byte_sum(catalog_entry.content) };
    }

    std::vector<size_t> directory_order(catalog.size());
    std::iota(directory_order.begin(), directory_order.end(), 0);
    std::sort(directory_order.begin(), directory_order.end(), [&entries](size_t left, size_t right) { return hash_less(entries[left], entries[right]); });
    for(size_t index = 1; index < directory_order.size(); ++index)
        if(!hash_less(entries[directory_order[index - 1]], entries[directory_order[index]]))
            return false;

    uint32_t virtual_position = kCatalogAreaSize;
    for(size_t ordinal = catalog.size() - 1; ordinal-- > 1;)
    {
        entries[ordinal].virtual_offset = virtual_position;
        virtual_position += entries[ordinal].size;
    }

    const uint64_t block_count_wide = (virtual_size + kAllocationBlockSize - 1) / kAllocationBlockSize;
    if(block_count_wide > std::numeric_limits<uint32_t>::max())
        return false;
    const uint32_t block_count = static_cast<uint32_t>(block_count_wide);
    const uint64_t padded_virtual_size = block_count_wide * kAllocationBlockSize;
    if(padded_virtual_size > std::numeric_limits<size_t>::max())
        return false;
    std::vector<uint8_t> virtual_data(static_cast<size_t>(padded_virtual_size));
    std::vector<size_t> directory_positions(catalog.size());
    for(size_t position = 0; position < directory_order.size(); ++position)
        directory_positions[directory_order[position]] = position;
    for(size_t ordinal = 0; ordinal < catalog.size(); ++ordinal)
    {
        const SfsCatalogEntry &catalog_entry = catalog[ordinal];
        const size_t metadata_offset = ordinal * kCatalogEntrySize;
        write_u32(virtual_data, metadata_offset, static_cast<uint32_t>(directory_positions[ordinal]));
        std::memcpy(virtual_data.data() + metadata_offset + 4, catalog_entry.name.data(), catalog_entry.name.size());
        write_u64(virtual_data, metadata_offset + 0x104, catalog_entry.creation_time);
        write_u64(virtual_data, metadata_offset + 0x10c, catalog_entry.access_time);
        write_u64(virtual_data, metadata_offset + 0x114, catalog_entry.write_time);
        std::memcpy(virtual_data.data() + metadata_offset + 0x11c, catalog_entry.name.data(), catalog_entry.name.size());
        if(!is_marker(catalog_entry.name))
            std::memcpy(virtual_data.data() + entries[ordinal].virtual_offset, catalog_entry.content.data(), catalog_entry.content.size());
    }

    const uint32_t directory_offset = kHeaderSize;
    const uint32_t allocation_map_offset = directory_offset + static_cast<uint32_t>(entries.size() * kDirectoryEntrySize);
    const uint64_t data_offset_wide = allocation_map_offset + (block_count_wide + 1) * sizeof(uint32_t);
    if(data_offset_wide > std::numeric_limits<uint32_t>::max())
        return false;
    const uint32_t data_offset = static_cast<uint32_t>(data_offset_wide);
    bytes.resize(data_offset);
    std::memcpy(bytes.data(), "SFS\0", 4);
    write_u32(bytes, 0x04, kVersion);
    write_u32(bytes, 0x0c, static_cast<uint32_t>(entries.size()));
    write_u32(bytes, 0x10, directory_offset);
    write_u32(bytes, 0x14, allocation_map_offset);
    write_u32(bytes, 0x18, data_offset);
    write_u32(bytes, 0x1c, static_cast<uint32_t>(virtual_size));
    write_u16(bytes, 0x20, static_cast<uint16_t>(maximum_path_length));
    std::memcpy(bytes.data() + kHeaderCommentOffset, metadata.header_comment.data(), metadata.header_comment.size());

    for(size_t position = 0; position < directory_order.size(); ++position)
    {
        const SfsEntry &entry = entries[directory_order[position]];
        const size_t offset = directory_offset + position * kDirectoryEntrySize;
        write_u32(bytes, offset, entry.hash_a);
        write_u32(bytes, offset + 4, entry.hash_b);
        write_u32(bytes, offset + 8, entry.attributes);
        write_u32(bytes, offset + 12, entry.virtual_offset);
        write_u32(bytes, offset + 16, entry.size);
        write_u32(bytes, offset + 20, entry.stored14);
        write_u32(bytes, offset + 24, entry.flags);
        write_u32(bytes, offset + 28, entry.stored1c);
    }

    freegag::LegacyGzipCompressionSession compression;
    std::vector<uint8_t> compressed(0x10000);
    for(uint32_t block_index = 0; block_index < block_count; ++block_index)
    {
        if(bytes.size() > std::numeric_limits<uint32_t>::max() - kAllocationBlockSize)
            return false;
        write_u32(bytes, allocation_map_offset + block_index * sizeof(uint32_t), static_cast<uint32_t>(bytes.size()));
        const uint8_t *source = virtual_data.data() + static_cast<size_t>(block_index) * kAllocationBlockSize;
        const uint32_t compressed_size = compression.compress(source, kAllocationBlockSize, compressed.data(), static_cast<uint32_t>(compressed.size()));
        if(compressed_size != 0 && compressed_size + 3 < kAllocationBlockSize)
        {
            bytes.push_back(2);
            bytes.push_back(8);
            bytes.push_back(0);
            bytes.insert(bytes.end(), compressed.begin(), compressed.begin() + compressed_size);
        }
        else
            bytes.insert(bytes.end(), source, source + kAllocationBlockSize);
    }
    if(bytes.size() > std::numeric_limits<uint32_t>::max())
        return false;
    write_u32(bytes, allocation_map_offset + block_count * sizeof(uint32_t), static_cast<uint32_t>(bytes.size()));

    uint32_t header_checksum = 0;
    for(size_t offset = 0; offset < kHeaderSize; ++offset)
        if(offset < 0x08 || offset >= 0x0c)
            header_checksum += bytes[offset];
    write_u32(bytes, 0x08, header_checksum);
    return true;
}

} // namespace xtet
