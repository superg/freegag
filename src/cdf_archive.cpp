#include "cdf_archive.h"
#include <cstring>
#include <fstream>
#include <limits>
#include <memory>
#include <new>
#include <vector>
#include <zlib.h>
#include "media.h"
#include "portable_path.h"
#include "portable_string.h"
#include "resource.h"
#include "shared_binary_file.h"

namespace freegag
{

struct CdfEntry
{
    uint8_t flags;
    char name[0x23];
    uint32_t file_offset;
    uint32_t uncompressed_size;
};

struct CdfArchive
{
    char signature[7];
    uint32_t index_size;
    void *entry_storage;
    uint32_t entry_count;
    std::shared_ptr<SharedBinaryFile> shared_file;
    uint32_t file_offset;
    AsyncFileRecord *archive_input_record;
    AsyncFileRecord *entry_stream_source_record;
    std::fstream output;
    uint32_t error;
    uint32_t write_entry_index;
    uint32_t index_data_size;
    CdfEntry *entries[2000];
};

// Inflates a raw DEFLATE stream through zlib.
int zlib_cdf_decompressor(const void *source, uint32_t source_size, void *destination)
{
    if(source == nullptr || destination == nullptr || source_size < 2)
        return -1;

    const auto *input = static_cast<const Bytef *>(source);
    if(input[0] == 0)
    {
        const uint32_t stored_size = source_size - 2;
        std::memcpy(destination, input + 2, stored_size);
        return static_cast<int>(stored_size);
    }
    if(input[0] != Z_DEFLATED)
        return -2;

    z_stream stream{};
    stream.next_in = const_cast<Bytef *>(input + 2);
    stream.avail_in = source_size - 2;
    stream.next_out = static_cast<Bytef *>(destination);
    stream.avail_out = std::numeric_limits<uInt>::max();
    if(inflateInit2(&stream, -MAX_WBITS) != Z_OK)
        return -1;

    const int inflate_result = inflate(&stream, Z_FINISH);
    const int result = inflate_result == Z_STREAM_END && stream.total_out <= static_cast<uLong>(std::numeric_limits<int>::max()) ? static_cast<int>(stream.total_out) : -1;
    inflateEnd(&stream);
    return result;
}

// Writes gzip-compressed data through zlib.
uint32_t zlib_cdf_compressor(const void *source, uint32_t source_size, void *destination, uint32_t destination_capacity)
{
    if((source == nullptr && source_size != 0) || destination == nullptr || destination_capacity < 2)
        return 0;

    auto *output = static_cast<Bytef *>(destination);
    output[0] = Z_DEFLATED;
    output[1] = 0;

    z_stream stream{};
    stream.next_in = static_cast<Bytef *>(const_cast<void *>(source));
    stream.avail_in = source_size;
    stream.next_out = output + 2;
    stream.avail_out = destination_capacity - 2;
    if(deflateInit2(&stream, 9, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK)
        return 0;

    const int deflate_result = deflate(&stream, Z_FINISH);
    const uint32_t result = deflate_result == Z_STREAM_END ? static_cast<uint32_t>(stream.total_out) + 2 : 0;
    deflateEnd(&stream);
    return result;
}


struct Cdf96aIndexStorage
{
    uint32_t reserved_0000;
    CdfEntry entries[0x100];
};

uint32_t cdf_last_error;

constexpr char cdf_96a_signature[] = "CDF96a";
constexpr char cdf_97a_signature[] = "CDF97a";
constexpr char cdf_96b_signature[] = "CDF96b";

bool signature_equals(const char *left, const char *right)
{
    for(int index = 0; index < 7; ++index)
        if(left[index] != right[index])
            return false;
    return true;
}

void seek_archive(CdfArchive *archive, uint32_t offset)
{
    if(archive->archive_input_record != nullptr)
    {
        set_async_file_position(archive->archive_input_record, offset);
        return;
    }
    if(archive->shared_file != nullptr && offset <= archive->shared_file->size())
        archive->file_offset = offset;
}

void read_archive(CdfArchive *archive, void *destination, uint32_t size, uint32_t *bytes_read)
{
    if(archive->archive_input_record != nullptr)
    {
        read_async_file_record(archive->archive_input_record, destination, size, bytes_read, 0);
        return;
    }
    *bytes_read = archive->shared_file->read_at(archive->file_offset, destination, size);
    archive->file_offset += *bytes_read;
}

uint32_t archive_input_size(CdfArchive *archive)
{
    return archive->archive_input_record != nullptr ? get_async_file_size(archive->archive_input_record) : archive->shared_file->size();
}

bool seek_archive_output(CdfArchive *archive, uint32_t offset)
{
    archive->output.clear();
    archive->output.seekp(static_cast<std::streamoff>(offset), std::ios::beg);
    return static_cast<bool>(archive->output);
}

uint32_t archive_output_position(CdfArchive *archive)
{
    const std::streampos position = archive->output.tellp();
    if(position == std::streampos(-1) || position > static_cast<std::streamoff>(UINT32_MAX))
        return 0;
    return static_cast<uint32_t>(position);
}

bool write_archive_output(CdfArchive *archive, const void *source, uint32_t size)
{
    archive->output.write(static_cast<const char *>(source), static_cast<std::streamsize>(size));
    return static_cast<bool>(archive->output);
}

void close_archive_handles(CdfArchive *archive)
{
    if(archive->archive_input_record != nullptr)
        close_async_file_record(archive->archive_input_record);
    if(archive->entry_stream_source_record != nullptr)
        close_async_file_record(archive->entry_stream_source_record);
    archive->archive_input_record = nullptr;
    archive->entry_stream_source_record = nullptr;
    archive->shared_file.reset();
    if(archive->output.is_open())
        archive->output.close();
}


CdfArchive *open_cdf_archive(const char *path, intptr_t alternate_stream)
{
    cdf_last_error = CDF_ERROR_FILE_OPEN_FAILED;
    cdf_last_error = CDF_ERROR_ALLOCATION_FAILED;
    auto *archive = new (std::nothrow) CdfArchive{};
    if(archive == nullptr)
        return nullptr;

    cdf_last_error = CDF_ERROR_FILE_OPEN_FAILED;
    if(alternate_stream == 0)
    {
        archive->shared_file = std::make_shared<SharedBinaryFile>(path);
        if(!archive->shared_file->is_open())
        {
            delete archive;
            return nullptr;
        }
    }
    else
    {
        auto *host = reinterpret_cast<AsyncFileHost *>(alternate_stream);
        archive->archive_input_record = open_async_file_record(host, path, 0, 0, 0);
        if(archive->archive_input_record == nullptr)
        {
            delete archive;
            return nullptr;
        }
        archive->entry_stream_source_record = open_async_file_record(host, path, 0, 0, 0);
        if(archive->entry_stream_source_record == nullptr)
        {
            close_archive_handles(archive);
            delete archive;
            return nullptr;
        }
    }

    char signature[7];
    uint32_t bytes_read;
    read_archive(archive, signature, sizeof(signature), &bytes_read);

    cdf_last_error = CDF_ERROR_INVALID_FORMAT;
    if(bytes_read != 7 || (!signature_equals(signature, cdf_96a_signature) && !signature_equals(signature, cdf_96b_signature) && !signature_equals(signature, cdf_97a_signature)))
    {
        close_archive_handles(archive);
        delete archive;
        return nullptr;
    }

    std::memcpy(archive->signature, signature, 7);
    if(initialize_cdf_index(archive) == 0)
    {
        cdf_last_error = archive->error;
        close_archive_handles(archive);
        delete archive;
        return nullptr;
    }
    cdf_last_error = CDF_ERROR_NONE;
    return archive;
}

const char *get_cdf_entry_name_by_index(CdfArchive *archive, uint32_t index)
{
    if(archive == nullptr)
        return nullptr;
    return archive->entries[index]->name;
}

uint8_t get_cdf_entry_flags(CdfArchive *archive, const char *name)
{
    if(archive == nullptr)
        return 0;

    for(uint32_t index = 0; index < archive->entry_count; ++index)
        if(compare_ascii_case_insensitive(archive->entries[index]->name, name) == 0)
            return archive->entries[index]->flags;
    return 0;
}

uint32_t get_cdf_entry_size(CdfArchive *archive, uint8_t selector, const char *name)
{
    if(archive == nullptr)
        return 0;

    for(uint32_t index = 0; index < archive->entry_count; ++index)
    {
        CdfEntry *entry = archive->entries[index];
        if((selector == 0 || entry->flags == selector) && compare_ascii_case_insensitive(entry->name, name) == 0)
            return entry->uncompressed_size;
    }
    return 0;
}

int read_uncompressed_cdf_entry(CdfArchive *archive, uint16_t entry_index, void *destination)
{
    CdfEntry *entry = archive->entries[entry_index];
    seek_archive(archive, entry->file_offset);
    uint32_t bytes_read;
    read_archive(archive, destination, entry->uncompressed_size, &bytes_read);

    if(bytes_read != entry->uncompressed_size)
    {
        archive->error = CDF_ERROR_READ_FAILED;
        return 0;
    }
    archive->error = CDF_ERROR_NONE;
    return 1;
}

int read_compressed_cdf_entry(CdfArchive *archive, uint16_t entry_index, void *destination)
{
    CdfEntry *entry = archive->entries[entry_index];
    uint32_t remaining_size = entry->uncompressed_size;
    uint32_t chunk_count = remaining_size >> 15;
    if((remaining_size & 0x7fff) != 0)
        ++chunk_count;
    ++chunk_count;

    archive->error = CDF_ERROR_ALLOCATION_FAILED;
    uint32_t offset_table_size = chunk_count << 2;
    std::vector<uint8_t> compressed_buffer(0x10000);
    std::vector<uint32_t> offsets(chunk_count);
    int result = 0;

    if(destination != nullptr)
    {
        archive->error = CDF_ERROR_READ_FAILED;
        seek_archive(archive, entry->file_offset);
        uint32_t bytes_read;
        read_archive(archive, offsets.data(), offset_table_size, &bytes_read);

        if(bytes_read == offset_table_size)
        {
            auto *output = static_cast<uint8_t *>(destination);
            uint32_t chunk_index = 0;
            uint32_t data_chunk_count = chunk_count - 1;
            while(chunk_index < data_chunk_count)
            {
                uint32_t compressed_size = offsets[chunk_index + 1] - offsets[chunk_index];
                read_archive(archive, compressed_buffer.data(), compressed_size, &bytes_read);
                if(bytes_read != compressed_size)
                    break;

                zlib_cdf_decompressor(compressed_buffer.data(), compressed_size, output);
                uint32_t output_size = remaining_size > 0x8000 ? 0x8000 : remaining_size;
                remaining_size -= output_size;
                output += output_size;
                ++chunk_index;
            }

            if(chunk_index == data_chunk_count)
            {
                result = 1;
                archive->error = CDF_ERROR_NONE;
            }
        }
    }

    return result;
}

int read_cdf_entry(CdfArchive *archive, uint8_t selector, const char *name, void *destination)
{
    if(archive == nullptr)
        return 0;

    uint16_t index = 0;
    while(index < archive->entry_count)
    {
        CdfEntry *entry = archive->entries[index];
        if((selector == 0 || entry->flags == selector) && compare_ascii_case_insensitive(entry->name, name) == 0)
        {
            if((entry->flags & CDF_ENTRY_COMPRESSED) == 0)
                return read_uncompressed_cdf_entry(archive, index, destination);
            return read_compressed_cdf_entry(archive, index, destination);
        }
        index = static_cast<uint16_t>(index + 1);
    }
    return 0;
}

uint32_t close_cdf_archive(CdfArchive *archive)
{
    if(archive == nullptr)
        return 0;

    delete[] static_cast<uint8_t *>(archive->entry_storage);
    close_archive_handles(archive);
    delete archive;
    return 1;
}

int initialize_cdf_index(CdfArchive *archive)
{
    if(archive == nullptr)
        return 0;

    int result = 0;
    const uint32_t file_size = archive_input_size(archive);
    uint32_t bytes_read = 0;

    if(signature_equals(archive->signature, cdf_97a_signature))
    {
        read_archive(archive, &archive->index_size, 4, &bytes_read);
        read_archive(archive, &archive->entry_count, 4, &bytes_read);
        read_archive(archive, &archive->index_data_size, 4, &bytes_read);

        uint32_t remaining_size = archive->entry_count * sizeof(CdfEntry);
        archive->error = CDF_ERROR_ALLOCATION_FAILED;
        archive->entry_storage = new (std::nothrow) uint8_t[remaining_size]{};
        if(archive->entry_storage == nullptr)
            return 0;
        for(uint32_t index = 0; index < archive->entry_count; ++index)
            archive->entries[index] = static_cast<CdfEntry *>(archive->entry_storage) + index;

        uint32_t chunk_count = remaining_size >> 15;
        if((remaining_size & 0x7fff) != 0)
            ++chunk_count;
        void *compressed_buffer = new (std::nothrow) uint8_t[0x10000]{};
        void *output_buffer = new (std::nothrow) uint8_t[0x8000]{};
        uint32_t offset_table_size = (chunk_count + 1) * sizeof(uint32_t);
        auto *offsets = reinterpret_cast<uint32_t *>(new (std::nothrow) uint8_t[offset_table_size]{});
        if(compressed_buffer != nullptr && output_buffer != nullptr && offsets != nullptr)
        {
            archive->error = CDF_ERROR_READ_FAILED;
            seek_archive(archive, file_size - archive->index_size);
            read_archive(archive, offsets, offset_table_size, &bytes_read);
            if(bytes_read == offset_table_size)
            {
                uint32_t output_offset = 0;
                uint32_t chunk_index = 0;
                while(chunk_index < chunk_count)
                {
                    uint32_t compressed_size = offsets[chunk_index + 1] - offsets[chunk_index];
                    read_archive(archive, compressed_buffer, compressed_size, &bytes_read);
                    if(bytes_read != compressed_size)
                        break;
                    zlib_cdf_decompressor(compressed_buffer, compressed_size, output_buffer);
                    uint32_t output_size = remaining_size > 0x8000 ? 0x8000 : remaining_size;
                    std::memcpy(static_cast<uint8_t *>(archive->entry_storage) + output_offset, output_buffer, output_size);
                    output_offset += output_size;
                    remaining_size -= output_size;
                    ++chunk_index;
                }
                if(chunk_index == chunk_count)
                {
                    result = 1;
                    archive->error = CDF_ERROR_NONE;
                }
            }
        }
        if(compressed_buffer != nullptr)
            delete[] static_cast<uint8_t *>(compressed_buffer);
        if(output_buffer != nullptr)
            delete[] static_cast<uint8_t *>(output_buffer);
        if(offsets != nullptr)
            delete[] reinterpret_cast<uint8_t *>(offsets);
    }
    else
    {
        archive->error = CDF_ERROR_READ_FAILED;
        seek_archive(archive, 7);
        read_archive(archive, &archive->index_size, 2, &bytes_read);
        if(bytes_read != 2)
            return 0;

        void *compressed_index = nullptr;
        if(signature_equals(archive->signature, cdf_96a_signature))
        {
            archive->error = CDF_ERROR_ALLOCATION_FAILED;
            archive->entry_count = 0x100;
            compressed_index = new (std::nothrow) uint8_t[archive->index_size]{};
            archive->entry_storage = new (std::nothrow) uint8_t[0x2c08]{};
            if(compressed_index != nullptr && archive->entry_storage != nullptr)
            {
                auto *storage = static_cast<Cdf96aIndexStorage *>(archive->entry_storage);
                for(uint32_t index = 0; index < archive->entry_count; ++index)
                    archive->entries[index] = &storage->entries[index];
            }
        }
        else if(signature_equals(archive->signature, cdf_96b_signature))
        {
            read_archive(archive, &archive->entry_count, 2, &bytes_read);
            read_archive(archive, &archive->index_data_size, 4, &bytes_read);
            archive->error = CDF_ERROR_ALLOCATION_FAILED;
            compressed_index = new (std::nothrow) uint8_t[archive->index_size]{};
            archive->entry_storage = new (std::nothrow) uint8_t[archive->entry_count * sizeof(CdfEntry)]{};
            if(compressed_index != nullptr && archive->entry_storage != nullptr)
                for(uint32_t index = 0; index < archive->entry_count; ++index)
                    archive->entries[index] = static_cast<CdfEntry *>(archive->entry_storage) + index;
        }

        if(compressed_index != nullptr && archive->entry_storage != nullptr)
        {
            seek_archive(archive, file_size - archive->index_size);
            read_archive(archive, compressed_index, archive->index_size, &bytes_read);
            archive->error = CDF_ERROR_READ_FAILED;
            if(bytes_read == archive->index_size)
            {
                archive->error = CDF_ERROR_NONE;
                zlib_cdf_decompressor(compressed_index, archive->index_size, archive->entry_storage);
                result = 1;
            }
        }
        if(compressed_index != nullptr)
            delete[] static_cast<uint8_t *>(compressed_index);
    }

    if(result == 0 && archive->entry_storage != nullptr)
    {
        delete[] static_cast<uint8_t *>(archive->entry_storage);
        archive->entry_storage = nullptr;
    }
    return result;
}

uint32_t write_uncompressed_cdf_entry(CdfArchive *archive, const void *data)
{
    CdfEntry *entry = archive->entries[archive->write_entry_index];
    uint32_t remaining = entry->uncompressed_size;
    uint32_t chunk_size = remaining < 0x200000 ? remaining : 0x200000;
    archive->error = CDF_ERROR_NONE;
    seek_archive_output(archive, entry->file_offset);
    const auto *source = static_cast<const uint8_t *>(data);
    while(remaining != 0)
    {
        if(!write_archive_output(archive, source, chunk_size))
        {
            archive->error = CDF_ERROR_WRITE_FAILED;
            return 0;
        }
        remaining -= chunk_size;
        source += chunk_size;
        if(remaining < 0x200000)
            chunk_size = remaining;
    }
    return 1;
}

uint32_t write_compressed_cdf_index(CdfArchive *archive)
{
    if(archive == nullptr)
        return 0;
    uint32_t result = 0;
    uint32_t remaining = archive->entry_count * sizeof(CdfEntry);
    const uint32_t block_count = (remaining >> 15) + ((remaining & 0x7fff) != 0 ? 1 : 0) + 1;
    archive->error = CDF_ERROR_ALLOCATION_FAILED;
    auto *source = reinterpret_cast<const uint8_t *>(archive->entries[0]);
    std::vector<uint8_t> compressed(0x10000);
    const uint32_t table_size = block_count * sizeof(uint32_t);
    std::vector<uint32_t> offsets(block_count);
    if(source != nullptr)
    {
        offsets[0] = table_size;
        archive->index_size = table_size;
        const uint32_t table_position = archive_output_position(archive);
        archive->error = CDF_ERROR_WRITE_FAILED;
        if(write_archive_output(archive, offsets.data(), table_size))
        {
            uint32_t cumulative = table_size;
            bool complete = true;
            for(uint32_t block = 0; block < block_count - 1; ++block)
            {
                const uint32_t block_size = remaining < 0x8000 ? remaining : 0x8000;
                const uint32_t compressed_size = zlib_cdf_compressor(source, block_size, compressed.data(), 0x10000);
                archive->index_size += compressed_size;
                if(!write_archive_output(archive, compressed.data(), compressed_size))
                {
                    complete = false;
                    break;
                }
                cumulative += compressed_size;
                source += 0x8000;
                offsets[block + 1] = cumulative;
                remaining -= block_size;
            }
            if(complete)
            {
                const uint32_t end_position = archive_output_position(archive);
                if(seek_archive_output(archive, table_position) && write_archive_output(archive, offsets.data(), table_size) && seek_archive_output(archive, end_position))
                {
                    result = 1;
                    archive->error = CDF_ERROR_NONE;
                }
            }
        }
    }
    return result;
}

uint32_t write_compressed_cdf_entry(CdfArchive *archive, const void *data)
{
    CdfEntry *entry = archive->entries[archive->write_entry_index];
    uint32_t remaining = entry->uncompressed_size;
    const uint32_t block_count = (remaining >> 15) + ((remaining & 0x7fff) != 0 ? 1 : 0) + 1;
    archive->error = CDF_ERROR_ALLOCATION_FAILED;
    uint32_t result = 0;
    std::vector<uint8_t> compressed(0x10000);
    const uint32_t table_size = block_count * sizeof(uint32_t);
    std::vector<uint32_t> offsets(block_count);
    const auto *source = static_cast<const uint8_t *>(data);
    if(source != nullptr)
    {
        offsets[0] = table_size;
        archive->error = CDF_ERROR_WRITE_FAILED;
        seek_archive_output(archive, entry->file_offset);
        if(write_archive_output(archive, offsets.data(), table_size))
        {
            uint32_t cumulative = table_size;
            bool complete = true;
            for(uint32_t block = 0; block < block_count - 1; ++block)
            {
                const uint32_t block_size = remaining < 0x8000 ? remaining : 0x8000;
                const uint32_t compressed_size = zlib_cdf_compressor(source, block_size, compressed.data(), 0x10000);
                if(!write_archive_output(archive, compressed.data(), compressed_size))
                {
                    complete = false;
                    break;
                }
                source += 0x8000;
                cumulative += compressed_size;
                offsets[block + 1] = cumulative;
                remaining -= block_size;
            }
            if(complete)
            {
                const uint32_t end_position = archive_output_position(archive);
                if(seek_archive_output(archive, entry->file_offset) && write_archive_output(archive, offsets.data(), table_size) && seek_archive_output(archive, end_position))
                {
                    result = 1;
                    archive->error = CDF_ERROR_NONE;
                }
            }
        }
    }
    return result;
}

uint32_t finalize_cdf_writer(CdfArchive *archive)
{
    if(archive == nullptr)
        return 0;
    archive->entry_count = archive->write_entry_index;
    write_compressed_cdf_index(archive);
    seek_archive_output(archive, 7);
    write_archive_output(archive, &archive->index_size, 4);
    write_archive_output(archive, &archive->write_entry_index, 4);
    write_archive_output(archive, &archive->index_data_size, 4);
    archive->output.flush();
    delete[] static_cast<uint8_t *>(archive->entry_storage);
    delete archive;
    return 0;
}

uint32_t append_cdf_writer_entry(CdfArchive *archive, const char *name, const void *data, uint32_t size, int compressed)
{
    if(archive == nullptr || archive->write_entry_index >= archive->entry_count)
        return 0;
    CdfEntry *entry = archive->entries[archive->write_entry_index];
    std::memcpy(entry->name, name, std::strlen(name) + 1);
    entry->flags = classify_runtime_media_data(data);
    entry->uncompressed_size = size;
    entry->file_offset = archive_output_position(archive);
    uint32_t result;
    if(compressed == 0)
    {
        result = write_uncompressed_cdf_entry(archive, data);
    }
    else
    {
        entry->flags = static_cast<uint8_t>(entry->flags | CDF_ENTRY_COMPRESSED);
        result = write_compressed_cdf_entry(archive, data);
    }
    ++archive->write_entry_index;
    archive->index_data_size += size;
    return result;
}

CdfArchive *create_cdf_writer(const char *path, uint32_t capacity)
{
    cdf_last_error = CDF_ERROR_ALLOCATION_FAILED;
    auto *archive = new (std::nothrow) CdfArchive{};
    if(archive == nullptr)
        return nullptr;
    cdf_last_error = CDF_ERROR_FILE_OPEN_FAILED;
    std::filesystem::path output_path;
    if(!resolve_existing_host_path_case_insensitive(path, &output_path))
        output_path = normalize_host_path(path);
    archive->output.open(output_path, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
    if(!archive->output)
    {
        delete archive;
        return nullptr;
    }
    cdf_last_error = CDF_ERROR_ALLOCATION_FAILED;
    archive->entry_storage = new (std::nothrow) uint8_t[capacity * sizeof(CdfEntry)]{};
    if(archive->entry_storage == nullptr)
    {
        delete archive;
        return nullptr;
    }
    std::memcpy(archive->signature, "CDF97a", 7);
    archive->entry_count = capacity & 0xffff;
    archive->write_entry_index = 0;
    archive->index_data_size = 0;
    archive->entries[0] = static_cast<CdfEntry *>(archive->entry_storage);
    for(uint32_t index = 1; index < archive->entry_count; ++index)
        archive->entries[index] = static_cast<CdfEntry *>(archive->entry_storage) + index;
    if(!write_archive_output(archive, archive->signature, 7) || !write_archive_output(archive, &archive->index_size, 4) || !write_archive_output(archive, &archive->entry_count, 4)
        || !write_archive_output(archive, &archive->index_data_size, 4))
    {
        delete[] static_cast<uint8_t *>(archive->entry_storage);
        delete archive;
        return nullptr;
    }
    cdf_last_error = CDF_ERROR_NONE;
    return archive;
}

uint32_t get_cdf_error(CdfArchive *archive)
{
    return archive == nullptr ? cdf_last_error : archive->error;
}

void *open_cdf_entry_async_record(CdfArchive *archive, AsyncFileHost *host, const char *name)
{
    if(archive == nullptr)
        return nullptr;
    for(uint32_t index = 0; index < archive->entry_count; ++index)
    {
        CdfEntry *entry = archive->entries[index];
        if(compare_ascii_case_insensitive(entry->name, name) != 0)
            continue;
        const uint32_t end = entry->file_offset + entry->uncompressed_size;
        if(archive->entry_stream_source_record != nullptr)
            return duplicate_async_file_record(archive->entry_stream_source_record, entry->file_offset, end, 0);
        return archive->shared_file == nullptr ? nullptr : open_async_file_record(host, archive->shared_file, entry->file_offset, end, 0);
    }
    return nullptr;
}

uint32_t write_comment_cdf_package(const char *path, const void *comment, const void *bitmap, const ScriptTextBuffer *configuration)
{
    CdfArchive *archive = create_cdf_writer(path, 3);
    if(archive == nullptr)
        return CDF_ERROR_ALLOCATION_FAILED;
    if(comment != nullptr)
    {
        const uint32_t size = static_cast<uint32_t>(std::strlen(static_cast<const char *>(comment)) + 1);
        if(append_cdf_writer_entry(archive, "COMMENT.TXT", comment, size, 0) == 0)
        {
            const uint32_t error = get_cdf_error(archive);
            finalize_cdf_writer(archive);
            return error;
        }
    }
    if(bitmap != nullptr)
    {
        const BitmapInfoHeader info_header = decode_bitmap_info_header(static_cast<const uint8_t *>(bitmap) + sizeof(BitmapFileHeader));
        if(info_header.biBitCount == 8)
        {
            const uint32_t pixel_offset = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader) + 0x100 * sizeof(BitmapColor);
            const uint32_t bitmap_size = static_cast<uint32_t>(info_header.biWidth * info_header.biHeight) + pixel_offset;
            if(append_cdf_writer_entry(archive, "COMMENT.BMP", bitmap, bitmap_size, 1) == 0)
            {
                const uint32_t error = get_cdf_error(archive);
                finalize_cdf_writer(archive);
                return error;
            }
        }
    }
    if(configuration != nullptr)
    {
        if(append_cdf_writer_entry(archive, "START.CFG", configuration->data, configuration->length, 1) == 0)
        {
            const uint32_t error = get_cdf_error(archive);
            finalize_cdf_writer(archive);
            return error;
        }
    }
    finalize_cdf_writer(archive);
    return CDF_ERROR_NONE;
}



} // namespace freegag
