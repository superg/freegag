#include "cdf_archive.h"
#include <cstring>
#include <limits>
#include <new>
#include <vector>
#include <zlib.h>
#include "binary_stream.h"
#include "media.h"
#include "portable_string.h"
#include "resource.h"

namespace gag
{

// Inflates a raw DEFLATE stream through zlib.
int zlib_cdf_decompressor(const void *source, uint32_t source_size, void *destination)
{
    if(source == nullptr || destination == nullptr || source_size < 2)
    {
        return -1;
    }

    const auto *input = static_cast<const Bytef *>(source);
    if(input[0] == 0)
    {
        const uint32_t stored_size = source_size - 2;
        std::memcpy(destination, input + 2, stored_size);
        return static_cast<int>(stored_size);
    }
    if(input[0] != Z_DEFLATED)
    {
        return -2;
    }

    z_stream stream{};
    stream.next_in = const_cast<Bytef *>(input + 2);
    stream.avail_in = source_size - 2;
    stream.next_out = static_cast<Bytef *>(destination);
    stream.avail_out = std::numeric_limits<uInt>::max();
    if(inflateInit2(&stream, -MAX_WBITS) != Z_OK)
    {
        return -1;
    }

    const int inflate_result = inflate(&stream, Z_FINISH);
    const int result = inflate_result == Z_STREAM_END && stream.total_out <= static_cast<uLong>(std::numeric_limits<int>::max()) ? static_cast<int>(stream.total_out) : -1;
    inflateEnd(&stream);
    return result;
}

// Writes gzip-compressed data through zlib.
uint32_t zlib_cdf_compressor(const void *source, uint32_t source_size, void *destination, uint32_t destination_capacity)
{
    if((source == nullptr && source_size != 0) || destination == nullptr || destination_capacity < 2)
    {
        return 0;
    }

    auto *output = static_cast<Bytef *>(destination);
    output[0] = Z_DEFLATED;
    output[1] = 0;

    z_stream stream{};
    stream.next_in = static_cast<Bytef *>(const_cast<void *>(source));
    stream.avail_in = source_size;
    stream.next_out = output + 2;
    stream.avail_out = destination_capacity - 2;
    if(deflateInit2(&stream, 9, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK)
    {
        return 0;
    }

    const int deflate_result = deflate(&stream, Z_FINISH);
    const uint32_t result = deflate_result == Z_STREAM_END ? static_cast<uint32_t>(stream.total_out) + 2 : 0;
    deflateEnd(&stream);
    return result;
}

namespace
{

struct Cdf96aIndexStorage
{
    uint32_t unknown_0000;
    CdfEntry entries[0x100];
};

class AsyncCdfInputStream final : public BinaryInputStream
{
public:
    AsyncCdfInputStream(intptr_t host, const char *path)
        : record_(open_async_file_record(reinterpret_cast<AsyncFileHost *>(host), path, 0, 0, 0))
    {
    }

    ~AsyncCdfInputStream() override
    {
        close_async_file_record(record_);
    }

    bool is_open() const
    {
        return record_ != nullptr;
    }

    AsyncFileRecord *record() const
    {
        return record_;
    }

    bool seek(uint32_t offset) override
    {
        return record_ != nullptr && set_async_file_position(record_, offset) != 0;
    }

    uint32_t size() override
    {
        return get_async_file_size(record_);
    }

    uint32_t read(void *destination, uint32_t size) override
    {
        uint32_t transferred = 0;
        read_async_file_record(record_, destination, size, &transferred, 0);
        return transferred;
    }

private:
    AsyncFileRecord *record_{};
};

int (*compressed_cdf_reader)(CdfArchive *, uint16_t, void *) = read_compressed_cdf_entry;
uint32_t cdf_last_error;

constexpr char cdf_96a_signature[] = "CDF96a";
constexpr char cdf_97a_signature[] = "CDF97a";
constexpr char cdf_96b_signature[] = "CDF96b";

bool signature_equals(const char *left, const char *right)
{
    for(int index = 0; index < 7; ++index)
    {
        if(left[index] != right[index])
        {
            return false;
        }
    }
    return true;
}

void seek_archive(CdfArchive *archive, uint32_t offset)
{
    archive->input->seek(offset);
}

void read_archive(CdfArchive *archive, void *destination, uint32_t size, uint32_t *bytes_read)
{
    *bytes_read = archive->input->read(destination, size);
}

void close_archive_handles(CdfArchive *archive)
{
    delete archive->input;
    delete archive->second_input;
    delete archive->output;
    archive->input = nullptr;
    archive->second_input = nullptr;
    archive->output = nullptr;
}

} // namespace

CdfArchive *open_cdf_archive(const char *path, intptr_t alternate_stream)
{
    cdf_last_error = 0x1000000;
    BinaryInputStream *input = nullptr;
    if(alternate_stream == 0)
    {
        auto *physical = new (std::nothrow) StandardBinaryInputStream(path);
        if(physical == nullptr || !physical->is_open())
        {
            delete physical;
            return nullptr;
        }
        input = physical;
    }
    else
    {
        auto *asynchronous = new (std::nothrow) AsyncCdfInputStream(alternate_stream, path);
        if(asynchronous == nullptr || !asynchronous->is_open())
        {
            delete asynchronous;
            return nullptr;
        }
        input = asynchronous;
    }

    cdf_last_error = 0x20000;
    auto *archive = new (std::nothrow) CdfArchive{};
    if(archive == nullptr)
    {
        delete input;
        return nullptr;
    }

    archive->input = input;
    archive->alternate_stream = alternate_stream;
    char signature[7];
    uint32_t bytes_read = input->read(signature, 7);
    if(alternate_stream != 0)
    {
        auto *second = new (std::nothrow) AsyncCdfInputStream(alternate_stream, path);
        if(second == nullptr || !second->is_open())
        {
            delete second;
            close_archive_handles(archive);
            delete archive;
            return nullptr;
        }
        archive->second_input = second;
    }

    cdf_last_error = 0x2000000;
    if(bytes_read != 7 || (!signature_equals(signature, cdf_96a_signature) && !signature_equals(signature, cdf_96b_signature) && !signature_equals(signature, cdf_97a_signature)))
    {
        close_archive_handles(archive);
        delete archive;
        return nullptr;
    }

    std::memcpy(archive->signature, signature, 7);
    size_t path_length = std::strlen(path) + 1;
    std::memcpy(archive->path, path, path_length);
    if(initialize_cdf_index(archive) == 0)
    {
        cdf_last_error = archive->error;
        close_archive_handles(archive);
        delete archive;
        return nullptr;
    }
    cdf_last_error = 0;
    return archive;
}

const char *get_cdf_entry_name_by_index(CdfArchive *archive, uint32_t index)
{
    if(archive == nullptr)
    {
        return nullptr;
    }
    return archive->entries[index]->name;
}

uint32_t get_cdf_entry_count(CdfArchive *archive)
{
    return archive == nullptr ? 0 : archive->entry_count;
}

uint32_t get_cdf_index_data_size(CdfArchive *archive)
{
    return archive == nullptr ? 0 : archive->index_data_size;
}

uint8_t get_cdf_entry_flags(CdfArchive *archive, const char *name)
{
    if(archive == nullptr)
    {
        return 0;
    }

    for(uint32_t index = 0; index < archive->entry_count; ++index)
    {
        if(compare_ascii_case_insensitive(archive->entries[index]->name, name) == 0)
        {
            return archive->entries[index]->flags;
        }
    }
    return 0;
}

uint32_t get_cdf_entry_size(CdfArchive *archive, uint8_t selector, const char *name)
{
    if(archive == nullptr)
    {
        return 0;
    }

    for(uint32_t index = 0; index < archive->entry_count; ++index)
    {
        CdfEntry *entry = archive->entries[index];
        if((selector == 0 || entry->flags == selector) && compare_ascii_case_insensitive(entry->name, name) == 0)
        {
            return entry->uncompressed_size;
        }
    }
    return 0;
}

int read_uncompressed_cdf_entry(CdfArchive *archive, uint16_t entry_index, void *destination)
{
    CdfEntry *entry = archive->entries[entry_index];
    archive->input->seek(entry->file_offset);
    const uint32_t bytes_read = archive->input->read(destination, entry->uncompressed_size);

    if(bytes_read != entry->uncompressed_size)
    {
        archive->error = 1;
        return 0;
    }
    archive->error = 0;
    return 1;
}

int read_compressed_cdf_entry(CdfArchive *archive, uint16_t entry_index, void *destination)
{
    CdfEntry *entry = archive->entries[entry_index];
    uint32_t remaining_size = entry->uncompressed_size;
    uint32_t chunk_count = remaining_size >> 15;
    if((remaining_size & 0x7fff) != 0)
    {
        ++chunk_count;
    }
    ++chunk_count;

    archive->error = 0x20000;
    uint32_t offset_table_size = chunk_count << 2;
    std::vector<uint8_t> compressed_buffer(0x10000);
    std::vector<uint32_t> offsets(chunk_count);
    int result = 0;

    if(destination != nullptr)
    {
        archive->error = 1;
        archive->input->seek(entry->file_offset);
        uint32_t bytes_read = archive->input->read(offsets.data(), offset_table_size);

        if(bytes_read == offset_table_size)
        {
            auto *output = static_cast<uint8_t *>(destination);
            uint32_t chunk_index = 0;
            uint32_t data_chunk_count = chunk_count - 1;
            while(chunk_index < data_chunk_count)
            {
                uint32_t compressed_size = offsets[chunk_index + 1] - offsets[chunk_index];
                bytes_read = archive->input->read(compressed_buffer.data(), compressed_size);
                if(bytes_read != compressed_size)
                {
                    break;
                }

                zlib_cdf_decompressor(compressed_buffer.data(), compressed_size, output);
                uint32_t output_size = remaining_size > 0x8000 ? 0x8000 : remaining_size;
                remaining_size -= output_size;
                output += output_size;
                ++chunk_index;
            }

            if(chunk_index == data_chunk_count)
            {
                result = 1;
                archive->error = 0;
            }
        }
    }

    return result;
}

int read_cdf_entry(CdfArchive *archive, uint8_t selector, const char *name, void *destination)
{
    if(archive == nullptr)
    {
        return 0;
    }

    uint16_t index = 0;
    while(index < archive->entry_count)
    {
        CdfEntry *entry = archive->entries[index];
        if((selector == 0 || entry->flags == selector) && compare_ascii_case_insensitive(entry->name, name) == 0)
        {
            if((entry->flags & 0x10) == 0)
            {
                return read_uncompressed_cdf_entry(archive, index, destination);
            }
            return compressed_cdf_reader(archive, index, destination);
        }
        index = static_cast<uint16_t>(index + 1);
    }
    return 0;
}

uint32_t close_cdf_archive(CdfArchive *archive)
{
    if(archive == nullptr)
    {
        return 0;
    }

    delete[] static_cast<uint8_t *>(archive->entry_storage);
    close_archive_handles(archive);
    delete archive;
    return 1;
}

int initialize_cdf_index(CdfArchive *archive)
{
    if(archive == nullptr)
    {
        return 0;
    }

    int result = 0;
    const uint32_t file_size = archive->input->size();
    uint32_t bytes_read = 0;

    if(signature_equals(archive->signature, cdf_97a_signature))
    {
        read_archive(archive, &archive->index_size, 4, &bytes_read);
        read_archive(archive, &archive->entry_count, 4, &bytes_read);
        read_archive(archive, &archive->index_data_size, 4, &bytes_read);

        uint32_t remaining_size = archive->entry_count * sizeof(CdfEntry);
        archive->error = 0x20000;
        archive->entry_storage = new (std::nothrow) uint8_t[remaining_size]{};
        if(archive->entry_storage == nullptr)
        {
            return 0;
        }
        for(uint32_t index = 0; index < archive->entry_count; ++index)
        {
            archive->entries[index] = static_cast<CdfEntry *>(archive->entry_storage) + index;
        }

        uint32_t chunk_count = remaining_size >> 15;
        if((remaining_size & 0x7fff) != 0)
        {
            ++chunk_count;
        }
        void *compressed_buffer = new (std::nothrow) uint8_t[0x10000]{};
        void *output_buffer = new (std::nothrow) uint8_t[0x8000]{};
        uint32_t offset_table_size = (chunk_count + 1) * sizeof(uint32_t);
        auto *offsets = reinterpret_cast<uint32_t *>(new (std::nothrow) uint8_t[offset_table_size]{});
        if(compressed_buffer != nullptr && output_buffer != nullptr && offsets != nullptr)
        {
            archive->error = 1;
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
                    {
                        break;
                    }
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
                    archive->error = 0;
                }
            }
        }
        if(compressed_buffer != nullptr)
        {
            delete[] static_cast<uint8_t *>(compressed_buffer);
        }
        if(output_buffer != nullptr)
        {
            delete[] static_cast<uint8_t *>(output_buffer);
        }
        if(offsets != nullptr)
        {
            delete[] reinterpret_cast<uint8_t *>(offsets);
        }
    }
    else
    {
        archive->error = 1;
        seek_archive(archive, 7);
        read_archive(archive, &archive->index_size, 2, &bytes_read);
        if(bytes_read != 2)
        {
            return 0;
        }

        void *compressed_index = nullptr;
        if(signature_equals(archive->signature, cdf_96a_signature))
        {
            archive->error = 0x20000;
            archive->entry_count = 0x100;
            compressed_index = new (std::nothrow) uint8_t[archive->index_size]{};
            archive->entry_storage = new (std::nothrow) uint8_t[0x2c08]{};
            if(compressed_index != nullptr && archive->entry_storage != nullptr)
            {
                auto *storage = static_cast<Cdf96aIndexStorage *>(archive->entry_storage);
                for(uint32_t index = 0; index < archive->entry_count; ++index)
                {
                    archive->entries[index] = &storage->entries[index];
                }
            }
        }
        else if(signature_equals(archive->signature, cdf_96b_signature))
        {
            read_archive(archive, &archive->entry_count, 2, &bytes_read);
            read_archive(archive, &archive->index_data_size, 4, &bytes_read);
            archive->error = 0x20000;
            compressed_index = new (std::nothrow) uint8_t[archive->index_size]{};
            archive->entry_storage = new (std::nothrow) uint8_t[archive->entry_count * sizeof(CdfEntry)]{};
            if(compressed_index != nullptr && archive->entry_storage != nullptr)
            {
                for(uint32_t index = 0; index < archive->entry_count; ++index)
                {
                    archive->entries[index] = static_cast<CdfEntry *>(archive->entry_storage) + index;
                }
            }
        }

        if(compressed_index != nullptr && archive->entry_storage != nullptr)
        {
            seek_archive(archive, file_size - archive->index_size);
            read_archive(archive, compressed_index, archive->index_size, &bytes_read);
            archive->error = 1;
            if(bytes_read == archive->index_size)
            {
                archive->error = 0;
                zlib_cdf_decompressor(compressed_index, archive->index_size, archive->entry_storage);
                result = 1;
            }
        }
        if(compressed_index != nullptr)
        {
            delete[] static_cast<uint8_t *>(compressed_index);
        }
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
    archive->error = 0;
    archive->output->seek(entry->file_offset);
    const auto *source = static_cast<const uint8_t *>(data);
    while(remaining != 0)
    {
        if(!archive->output->write(source, chunk_size))
        {
            archive->error = 2;
            return 0;
        }
        remaining -= chunk_size;
        source += chunk_size;
        if(remaining < 0x200000)
        {
            chunk_size = remaining;
        }
    }
    return 1;
}

uint32_t write_compressed_cdf_index(CdfArchive *archive)
{
    if(archive == nullptr)
    {
        return 0;
    }
    uint32_t result = 0;
    uint32_t remaining = archive->entry_count * sizeof(CdfEntry);
    const uint32_t block_count = (remaining >> 15) + ((remaining & 0x7fff) != 0 ? 1 : 0) + 1;
    archive->error = 0x20000;
    auto *source = reinterpret_cast<const uint8_t *>(archive->entries[0]);
    std::vector<uint8_t> compressed(0x10000);
    const uint32_t table_size = block_count * sizeof(uint32_t);
    std::vector<uint32_t> offsets(block_count);
    if(source != nullptr)
    {
        offsets[0] = table_size;
        archive->index_size = table_size;
        const uint32_t table_position = archive->output->position();
        archive->error = 2;
        if(archive->output->write(offsets.data(), table_size))
        {
            uint32_t cumulative = table_size;
            bool complete = true;
            for(uint32_t block = 0; block < block_count - 1; ++block)
            {
                const uint32_t block_size = remaining < 0x8000 ? remaining : 0x8000;
                const uint32_t compressed_size = zlib_cdf_compressor(source, block_size, compressed.data(), 0x10000);
                archive->index_size += compressed_size;
                if(!archive->output->write(compressed.data(), compressed_size))
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
                const uint32_t end_position = archive->output->position();
                if(archive->output->seek(table_position) && archive->output->write(offsets.data(), table_size) && archive->output->seek(end_position))
                {
                    result = 1;
                    archive->error = 0;
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
    archive->error = 0x20000;
    uint32_t result = 0;
    std::vector<uint8_t> compressed(0x10000);
    const uint32_t table_size = block_count * sizeof(uint32_t);
    std::vector<uint32_t> offsets(block_count);
    const auto *source = static_cast<const uint8_t *>(data);
    if(source != nullptr)
    {
        offsets[0] = table_size;
        archive->error = 2;
        archive->output->seek(entry->file_offset);
        if(archive->output->write(offsets.data(), table_size))
        {
            uint32_t cumulative = table_size;
            bool complete = true;
            for(uint32_t block = 0; block < block_count - 1; ++block)
            {
                const uint32_t block_size = remaining < 0x8000 ? remaining : 0x8000;
                const uint32_t compressed_size = zlib_cdf_compressor(source, block_size, compressed.data(), 0x10000);
                if(!archive->output->write(compressed.data(), compressed_size))
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
                const uint32_t end_position = archive->output->position();
                if(archive->output->seek(entry->file_offset) && archive->output->write(offsets.data(), table_size) && archive->output->seek(end_position))
                {
                    result = 1;
                    archive->error = 0;
                }
            }
        }
    }
    return result;
}

uint32_t finalize_cdf_writer(CdfArchive *archive)
{
    if(archive == nullptr)
    {
        return 0;
    }
    archive->entry_count = archive->write_entry_index;
    write_compressed_cdf_index(archive);
    archive->output->seek(7);
    archive->output->write(&archive->index_size, 4);
    archive->output->write(&archive->write_entry_index, 4);
    archive->output->write(&archive->index_data_size, 4);
    archive->output->flush();
    delete archive->output;
    delete[] static_cast<uint8_t *>(archive->entry_storage);
    delete archive;
    return 0;
}

uint32_t append_cdf_writer_entry(CdfArchive *archive, const char *name, const void *data, uint32_t size, int compressed)
{
    if(archive == nullptr || archive->write_entry_index >= archive->entry_count)
    {
        return 0;
    }
    CdfEntry *entry = archive->entries[archive->write_entry_index];
    std::memcpy(entry->name, name, std::strlen(name) + 1);
    entry->flags = classify_runtime_media_data(data);
    entry->uncompressed_size = size;
    entry->file_offset = archive->output->position();
    uint32_t result;
    if(compressed == 0)
    {
        result = write_uncompressed_cdf_entry(archive, data);
    }
    else
    {
        entry->flags = static_cast<uint8_t>(entry->flags + 0x10);
        result = write_compressed_cdf_entry(archive, data);
    }
    ++archive->write_entry_index;
    archive->index_data_size += size;
    return result;
}

CdfArchive *create_cdf_writer(const char *path, uint32_t capacity)
{
    cdf_last_error = 0x1000000;
    auto *output = new (std::nothrow) StandardBinaryOutputStream(path);
    if(output == nullptr || !output->is_open())
    {
        delete output;
        return nullptr;
    }
    cdf_last_error = 0x20000;
    auto *archive = new (std::nothrow) CdfArchive{};
    if(archive == nullptr)
    {
        delete output;
        return nullptr;
    }
    archive->entry_storage = new (std::nothrow) uint8_t[capacity * sizeof(CdfEntry)]{};
    if(archive->entry_storage == nullptr)
    {
        delete archive;
        delete output;
        return nullptr;
    }
    std::memcpy(archive->signature, "CDF97a", 7);
    archive->entry_count = capacity & 0xffff;
    archive->write_entry_index = 0;
    archive->index_data_size = 0;
    archive->entries[0] = static_cast<CdfEntry *>(archive->entry_storage);
    for(uint32_t index = 1; index < archive->entry_count; ++index)
    {
        archive->entries[index] = static_cast<CdfEntry *>(archive->entry_storage) + index;
    }
    archive->output = output;
    if(!output->write(archive->signature, 7) || !output->write(&archive->index_size, 4) || !output->write(&archive->entry_count, 4) || !output->write(&archive->index_data_size, 4))
    {
        delete output;
        delete[] static_cast<uint8_t *>(archive->entry_storage);
        delete archive;
        return nullptr;
    }
    cdf_last_error = 0;
    return archive;
}

uint32_t get_cdf_error(CdfArchive *archive)
{
    return archive == nullptr ? cdf_last_error : archive->error;
}

void *open_cdf_entry_async_record(CdfArchive *archive, AsyncFileHost *host, uint32_t start, uint32_t end)
{
    if(auto *input = dynamic_cast<AsyncCdfInputStream *>(archive->second_input))
    {
        return duplicate_async_file_record(input->record(), start, end, 0);
    }
    auto *input = dynamic_cast<StandardBinaryInputStream *>(archive->input);
    return input == nullptr ? nullptr : open_async_file_record(host, input->shared_state(), start, end, 0);
}

uint32_t write_comment_cdf_package(const char *path, const void *comment, const void *bitmap, const ScriptTextBuffer *configuration)
{
    CdfArchive *archive = create_cdf_writer(path, 3);
    if(archive == nullptr)
    {
        return 0x20000;
    }
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
    return 0;
}



} // namespace gag
