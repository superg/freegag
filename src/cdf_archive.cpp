#include "cdf_archive.h"
#include <cstring>
#include "startup.h"

namespace gag
{
namespace
{

void __fastcall seek_async_cdf_stream(HANDLE stream, std::uint32_t offset)
{
    set_async_file_position(reinterpret_cast<AsyncFileRecord *>(stream), offset);
}

void __fastcall read_async_cdf_stream(HANDLE stream, void *destination, std::uint32_t size, DWORD *bytes_read)
{
    std::uint32_t transferred;
    read_async_file_record(reinterpret_cast<AsyncFileRecord *>(stream), destination, size, &transferred, 0);
    *bytes_read = transferred;
}

CdfIoApi cdf_io_api{ SetFilePointer, ReadFile, seek_async_cdf_stream, read_async_cdf_stream };

int(__fastcall *compressed_cdf_reader)(CdfArchive *, std::uint16_t, void *) = read_compressed_cdf_entry;

BOOL __fastcall close_async_cdf_stream(HANDLE stream)
{
    return static_cast<BOOL>(close_async_file_record(reinterpret_cast<AsyncFileRecord *>(stream)));
}

CdfLifecycleApi cdf_lifecycle_api{ CloseHandle, close_async_cdf_stream, GetProcessHeap, HeapFree };

// Non-original library boundary. GAG.EXE delegates this operation to its bundled inflate implementation rooted at 0x0040F8D0.
int __fastcall unavailable_bundled_cdf_decompressor(const void *, std::uint32_t, void *)
{
    return -2;
}

CdfCompressionApi cdf_compression_api{ GetProcessHeap, HeapAlloc, HeapFree, unavailable_bundled_cdf_decompressor };

std::uint32_t __fastcall get_async_cdf_stream_size(HANDLE stream)
{
    return get_async_file_size(reinterpret_cast<AsyncFileRecord *>(stream));
}

CdfIndexApi cdf_index_api{ GetFileSize, get_async_cdf_stream_size };

HANDLE __fastcall open_async_cdf_stream(std::uint32_t host, const char *path)
{
    return reinterpret_cast<HANDLE>(open_async_file_record(reinterpret_cast<AsyncFileHost *>(host), path, 0, 0, 0));
}

CdfOpenApi cdf_open_api{ CreateFileA, open_async_cdf_stream };
CdfWriteApi cdf_write_api{ GetProcessHeap, SetFilePointer, WriteFile };

// Non-original library boundary. GAG.EXE delegates this operation to its bundled GNU gzip implementation at 0x00418E90.
std::uint32_t __fastcall unavailable_bundled_gzip_compressor(const void *, std::uint32_t, void *, std::uint32_t)
{
    return 0;
}

CdfCompressedWriteApi cdf_compressed_write_api{ GetProcessHeap, HeapAlloc, HeapFree, SetFilePointer, WriteFile, unavailable_bundled_gzip_compressor };
CdfWriterFinalizeApi cdf_writer_finalize_api{ write_compressed_cdf_index, SetFilePointer, WriteFile, GetProcessHeap, CloseHandle, HeapFree };

CdfEntryWriteApi cdf_entry_write_api{ classify_runtime_media_data, SetFilePointer, write_uncompressed_cdf_entry, write_compressed_cdf_entry };
CdfWriterCreateApi cdf_writer_create_api{ CreateFileA, GetLastError, GetProcessHeap, HeapAlloc, HeapFree, CloseHandle, WriteFile };
CdfCommentPackageApi cdf_comment_package_api{ create_cdf_writer, append_cdf_writer_entry, get_cdf_error, finalize_cdf_writer };
std::uint32_t cdf_last_error;

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

void seek_archive(CdfArchive *archive, std::uint32_t offset)
{
    if(archive->alternate_stream == 0)
    {
        cdf_io_api.set_file_pointer(archive->handle, static_cast<LONG>(offset), nullptr, FILE_BEGIN);
    }
    else
    {
        cdf_io_api.seek_alternate_stream(archive->handle, offset);
    }
}

void read_archive(CdfArchive *archive, void *destination, std::uint32_t size, DWORD *bytes_read)
{
    if(archive->alternate_stream == 0)
    {
        cdf_io_api.read_file(archive->handle, destination, size, bytes_read, nullptr);
    }
    else
    {
        cdf_io_api.read_alternate_stream(archive->handle, destination, size, bytes_read);
    }
}

void close_archive_handles(CdfArchive *archive)
{
    if(archive->alternate_stream == 0)
    {
        cdf_lifecycle_api.close_handle(archive->handle);
    }
    else
    {
        cdf_lifecycle_api.close_alternate_stream(archive->handle);
        cdf_lifecycle_api.close_alternate_stream(archive->second_handle);
    }
}

} // namespace

// GAG.EXE: 0x004282A0
CdfArchive *__fastcall open_cdf_archive(const char *path, int alternate_stream)
{
    cdf_last_error = 0x1000000;
    HANDLE handle;
    if(alternate_stream == 0)
    {
        handle = cdf_open_api.create_file(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if(handle == INVALID_HANDLE_VALUE)
        {
            return nullptr;
        }
    }
    else
    {
        handle = cdf_open_api.open_alternate_stream(static_cast<std::uint32_t>(alternate_stream), path);
        if(handle == nullptr)
        {
            return nullptr;
        }
    }

    cdf_last_error = 0x20000;
    HANDLE heap = cdf_compression_api.get_process_heap();
    auto *archive = static_cast<CdfArchive *>(cdf_compression_api.heap_alloc(heap, HEAP_ZERO_MEMORY, sizeof(CdfArchive)));
    if(archive == nullptr)
    {
        if(alternate_stream == 0)
        {
            cdf_lifecycle_api.close_handle(handle);
        }
        else
        {
            cdf_lifecycle_api.close_alternate_stream(handle);
        }
        return nullptr;
    }

    archive->handle = handle;
    char signature[7];
    DWORD bytes_read;
    if(alternate_stream == 0)
    {
        cdf_io_api.read_file(handle, signature, 7, &bytes_read, nullptr);
    }
    else
    {
        archive->alternate_stream = alternate_stream;
        archive->second_handle = cdf_open_api.open_alternate_stream(static_cast<std::uint32_t>(alternate_stream), path);
        cdf_io_api.read_alternate_stream(handle, signature, 7, &bytes_read);
    }

    cdf_last_error = 0x2000000;
    if(!signature_equals(signature, cdf_96a_signature) && !signature_equals(signature, cdf_96b_signature) && !signature_equals(signature, cdf_97a_signature))
    {
        close_archive_handles(archive);
        cdf_compression_api.heap_free(heap, 0, archive);
        return nullptr;
    }

    std::memcpy(archive->signature, signature, 7);
    std::size_t path_length = std::strlen(path) + 1;
    std::memcpy(archive->path, path, path_length);
    if(initialize_cdf_index(archive) == 0)
    {
        cdf_last_error = archive->error;
        close_archive_handles(archive);
        cdf_compression_api.heap_free(heap, 0, archive);
        return nullptr;
    }
    cdf_last_error = 0;
    return archive;
}

// GAG.EXE: 0x00428620
const char *__fastcall get_cdf_entry_name_by_index(CdfArchive *archive, std::uint32_t index)
{
    if(archive == nullptr)
    {
        return nullptr;
    }
    return archive->entries[index]->name;
}

// GAG.EXE: 0x00428690
std::uint32_t __fastcall get_cdf_entry_count(CdfArchive *archive)
{
    return archive == nullptr ? 0 : archive->entry_count;
}

// GAG.EXE: 0x00428710
std::uint32_t __fastcall get_cdf_index_data_size(CdfArchive *archive)
{
    return archive == nullptr ? 0 : archive->index_data_size;
}

// GAG.EXE: 0x00428630
std::uint8_t __fastcall get_cdf_entry_flags(CdfArchive *archive, const char *name)
{
    if(archive == nullptr)
    {
        return 0;
    }

    for(std::uint32_t index = 0; index < archive->entry_count; ++index)
    {
        if(lstrcmpiA(archive->entries[index]->name, name) == 0)
        {
            return archive->entries[index]->flags;
        }
    }
    return 0;
}

// GAG.EXE: 0x004286A0
std::uint32_t __fastcall get_cdf_entry_size(CdfArchive *archive, std::uint8_t selector, const char *name)
{
    if(archive == nullptr)
    {
        return 0;
    }

    for(std::uint32_t index = 0; index < archive->entry_count; ++index)
    {
        CdfEntry *entry = archive->entries[index];
        if((selector == 0 || entry->flags == selector) && lstrcmpiA(entry->name, name) == 0)
        {
            return entry->uncompressed_size;
        }
    }
    return 0;
}

// GAG.EXE: 0x00429320
int __fastcall read_uncompressed_cdf_entry(CdfArchive *archive, std::uint16_t entry_index, void *destination)
{
    CdfEntry *entry = archive->entries[entry_index];
    DWORD bytes_read;
    if(archive->alternate_stream == 0)
    {
        cdf_io_api.set_file_pointer(archive->handle, static_cast<LONG>(entry->file_offset), nullptr, FILE_BEGIN);
        cdf_io_api.read_file(archive->handle, destination, entry->uncompressed_size, &bytes_read, nullptr);
    }
    else
    {
        cdf_io_api.seek_alternate_stream(archive->handle, entry->file_offset);
        cdf_io_api.read_alternate_stream(archive->handle, destination, entry->uncompressed_size, &bytes_read);
    }

    if(bytes_read != entry->uncompressed_size)
    {
        archive->error = 1;
        return 0;
    }
    archive->error = 0;
    return 1;
}

// GAG.EXE: 0x004293D0
int __fastcall read_compressed_cdf_entry(CdfArchive *archive, std::uint16_t entry_index, void *destination)
{
    CdfEntry *entry = archive->entries[entry_index];
    std::uint32_t remaining_size = entry->uncompressed_size;
    std::uint32_t chunk_count = remaining_size >> 15;
    if((remaining_size & 0x7fff) != 0)
    {
        ++chunk_count;
    }
    ++chunk_count;

    HANDLE heap = cdf_compression_api.get_process_heap();
    archive->error = 0x20000;
    void *compressed_buffer = cdf_compression_api.heap_alloc(heap, 0, 0x10000);
    std::uint32_t offset_table_size = chunk_count << 2;
    auto *offsets = static_cast<std::uint32_t *>(cdf_compression_api.heap_alloc(heap, 0, offset_table_size));
    int result = 0;

    if(compressed_buffer != nullptr && destination != nullptr && offsets != nullptr)
    {
        archive->error = 1;
        DWORD bytes_read;
        if(archive->alternate_stream == 0)
        {
            cdf_io_api.set_file_pointer(archive->handle, static_cast<LONG>(entry->file_offset), nullptr, FILE_BEGIN);
            cdf_io_api.read_file(archive->handle, offsets, offset_table_size, &bytes_read, nullptr);
        }
        else
        {
            cdf_io_api.seek_alternate_stream(archive->handle, entry->file_offset);
            cdf_io_api.read_alternate_stream(archive->handle, offsets, offset_table_size, &bytes_read);
        }

        if(bytes_read == offset_table_size)
        {
            auto *output = static_cast<std::uint8_t *>(destination);
            std::uint32_t chunk_index = 0;
            std::uint32_t data_chunk_count = chunk_count - 1;
            while(chunk_index < data_chunk_count)
            {
                std::uint32_t compressed_size = offsets[chunk_index + 1] - offsets[chunk_index];
                if(archive->alternate_stream == 0)
                {
                    cdf_io_api.read_file(archive->handle, compressed_buffer, compressed_size, &bytes_read, nullptr);
                }
                else
                {
                    cdf_io_api.read_alternate_stream(archive->handle, compressed_buffer, compressed_size, &bytes_read);
                }
                if(bytes_read != compressed_size)
                {
                    break;
                }

                cdf_compression_api.decompress(compressed_buffer, compressed_size, output);
                std::uint32_t output_size = remaining_size > 0x8000 ? 0x8000 : remaining_size;
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

    if(compressed_buffer != nullptr)
    {
        cdf_compression_api.heap_free(heap, 0, compressed_buffer);
    }
    if(offsets != nullptr)
    {
        cdf_compression_api.heap_free(heap, 0, offsets);
    }
    return result;
}

// GAG.EXE: 0x004284E0
int __fastcall read_cdf_entry(CdfArchive *archive, std::uint8_t selector, const char *name, void *destination)
{
    if(archive == nullptr)
    {
        return 0;
    }

    std::uint16_t index = 0;
    while(index < archive->entry_count)
    {
        CdfEntry *entry = archive->entries[index];
        if((selector == 0 || entry->flags == selector) && lstrcmpiA(entry->name, name) == 0)
        {
            if((entry->flags & 0x10) == 0)
            {
                return read_uncompressed_cdf_entry(archive, index, destination);
            }
            return compressed_cdf_reader(archive, index, destination);
        }
        index = static_cast<std::uint16_t>(index + 1);
    }
    return 0;
}

// GAG.EXE: 0x00428590
std::uint32_t __fastcall close_cdf_archive(CdfArchive *archive)
{
    if(archive == nullptr)
    {
        return 0;
    }

    std::uint32_t result = 1;
    if(archive->entry_storage != nullptr)
    {
        result = cdf_lifecycle_api.heap_free(cdf_lifecycle_api.get_process_heap(), 0, archive->entry_storage) & 1;
    }

    std::uint32_t close_result;
    if(archive->alternate_stream == 0)
    {
        close_result = cdf_lifecycle_api.close_handle(archive->handle);
    }
    else
    {
        result &= cdf_lifecycle_api.close_alternate_stream(archive->handle);
        close_result = cdf_lifecycle_api.close_alternate_stream(archive->second_handle);
    }

    std::uint32_t free_result = cdf_lifecycle_api.heap_free(cdf_lifecycle_api.get_process_heap(), 0, archive);
    return result & close_result & free_result;
}

// GAG.EXE: 0x004287E0
int __fastcall initialize_cdf_index(CdfArchive *archive)
{
    if(archive == nullptr)
    {
        return 0;
    }

    int result = 0;
    DWORD file_size = archive->alternate_stream == 0 ? cdf_index_api.get_file_size(archive->handle, nullptr) : cdf_index_api.get_alternate_stream_size(archive->handle);
    HANDLE heap = cdf_compression_api.get_process_heap();
    DWORD bytes_read = 0;

    if(signature_equals(archive->signature, cdf_97a_signature))
    {
        read_archive(archive, &archive->index_size, 4, &bytes_read);
        read_archive(archive, &archive->entry_count, 4, &bytes_read);
        read_archive(archive, &archive->index_data_size, 4, &bytes_read);

        std::uint32_t remaining_size = archive->entry_count * sizeof(CdfEntry);
        archive->error = 0x20000;
        archive->entry_storage = cdf_compression_api.heap_alloc(heap, HEAP_ZERO_MEMORY, remaining_size);
        if(archive->entry_storage == nullptr)
        {
            return 0;
        }
        for(std::uint32_t index = 0; index < archive->entry_count; ++index)
        {
            archive->entries[index] = static_cast<CdfEntry *>(archive->entry_storage) + index;
        }

        std::uint32_t chunk_count = remaining_size >> 15;
        if((remaining_size & 0x7fff) != 0)
        {
            ++chunk_count;
        }
        void *compressed_buffer = cdf_compression_api.heap_alloc(heap, HEAP_ZERO_MEMORY, 0x10000);
        void *output_buffer = cdf_compression_api.heap_alloc(heap, HEAP_ZERO_MEMORY, 0x8000);
        std::uint32_t offset_table_size = (chunk_count + 1) * sizeof(std::uint32_t);
        auto *offsets = static_cast<std::uint32_t *>(cdf_compression_api.heap_alloc(heap, HEAP_ZERO_MEMORY, offset_table_size));
        if(compressed_buffer != nullptr && output_buffer != nullptr && offsets != nullptr)
        {
            archive->error = 1;
            seek_archive(archive, file_size - archive->index_size);
            read_archive(archive, offsets, offset_table_size, &bytes_read);
            if(bytes_read == offset_table_size)
            {
                std::uint32_t output_offset = 0;
                std::uint32_t chunk_index = 0;
                while(chunk_index < chunk_count)
                {
                    std::uint32_t compressed_size = offsets[chunk_index + 1] - offsets[chunk_index];
                    read_archive(archive, compressed_buffer, compressed_size, &bytes_read);
                    if(bytes_read != compressed_size)
                    {
                        break;
                    }
                    cdf_compression_api.decompress(compressed_buffer, compressed_size, output_buffer);
                    std::uint32_t output_size = remaining_size > 0x8000 ? 0x8000 : remaining_size;
                    std::memcpy(static_cast<std::uint8_t *>(archive->entry_storage) + output_offset, output_buffer, output_size);
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
            cdf_compression_api.heap_free(heap, 0, compressed_buffer);
        }
        if(output_buffer != nullptr)
        {
            cdf_compression_api.heap_free(heap, 0, output_buffer);
        }
        if(offsets != nullptr)
        {
            cdf_compression_api.heap_free(heap, 0, offsets);
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
            compressed_index = cdf_compression_api.heap_alloc(heap, HEAP_ZERO_MEMORY, archive->index_size);
            archive->entry_storage = cdf_compression_api.heap_alloc(heap, HEAP_ZERO_MEMORY, 0x2c08);
            if(compressed_index != nullptr && archive->entry_storage != nullptr)
            {
                for(std::uint32_t index = 0; index < archive->entry_count; ++index)
                {
                    archive->entries[index] = reinterpret_cast<CdfEntry *>(static_cast<std::uint8_t *>(archive->entry_storage) + 4 + index * sizeof(CdfEntry));
                }
            }
        }
        else if(signature_equals(archive->signature, cdf_96b_signature))
        {
            read_archive(archive, &archive->entry_count, 2, &bytes_read);
            read_archive(archive, &archive->index_data_size, 4, &bytes_read);
            archive->error = 0x20000;
            compressed_index = cdf_compression_api.heap_alloc(heap, HEAP_ZERO_MEMORY, archive->index_size);
            archive->entry_storage = cdf_compression_api.heap_alloc(heap, HEAP_ZERO_MEMORY, archive->entry_count * sizeof(CdfEntry));
            if(compressed_index != nullptr && archive->entry_storage != nullptr)
            {
                for(std::uint32_t index = 0; index < archive->entry_count; ++index)
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
                cdf_compression_api.decompress(compressed_index, archive->index_size, archive->entry_storage);
                result = 1;
            }
        }
        if(compressed_index != nullptr)
        {
            cdf_compression_api.heap_free(heap, 0, compressed_index);
        }
    }

    if(result == 0 && archive->entry_storage != nullptr)
    {
        cdf_compression_api.heap_free(heap, 0, archive->entry_storage);
        archive->entry_storage = nullptr;
    }
    return result;
}

// GAG.EXE: 0x00429A90
std::uint32_t __fastcall write_uncompressed_cdf_entry(CdfArchive *archive, const void *data)
{
    CdfEntry *entry = archive->entries[archive->write_entry_index];
    std::uint32_t remaining = entry->uncompressed_size;
    DWORD chunk_size = remaining < 0x200000 ? remaining : 0x200000;
    cdf_write_api.get_process_heap();
    archive->error = 0;
    cdf_write_api.set_file_pointer(archive->handle, static_cast<LONG>(entry->file_offset), nullptr, FILE_BEGIN);
    const auto *source = static_cast<const std::uint8_t *>(data);
    while(remaining != 0)
    {
        DWORD bytes_written;
        cdf_write_api.write_file(archive->handle, source, chunk_size, &bytes_written, nullptr);
        if(bytes_written != chunk_size)
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

// GAG.EXE: 0x00429070
std::uint32_t __fastcall write_compressed_cdf_index(CdfArchive *archive)
{
    if(archive == nullptr)
    {
        return 0;
    }
    std::uint32_t result = 0;
    HANDLE heap = cdf_compressed_write_api.get_process_heap();
    std::uint32_t remaining = archive->entry_count * sizeof(CdfEntry);
    const std::uint32_t block_count = (remaining >> 15) + ((remaining & 0x7fff) != 0 ? 1 : 0) + 1;
    archive->error = 0x20000;
    auto *source = reinterpret_cast<const std::uint8_t *>(archive->entries[0]);
    auto *compressed = static_cast<std::uint8_t *>(cdf_compressed_write_api.heap_alloc(heap, HEAP_ZERO_MEMORY, 0x10000));
    const DWORD table_size = block_count * sizeof(std::uint32_t);
    auto *offsets = static_cast<std::uint32_t *>(cdf_compressed_write_api.heap_alloc(heap, HEAP_ZERO_MEMORY, table_size));
    if(source != nullptr && compressed != nullptr && offsets != nullptr)
    {
        offsets[0] = table_size;
        archive->index_size = table_size;
        const DWORD table_position = cdf_compressed_write_api.set_file_pointer(archive->handle, 0, nullptr, FILE_CURRENT);
        archive->error = 2;
        DWORD bytes_written;
        cdf_compressed_write_api.write_file(archive->handle, offsets, table_size, &bytes_written, nullptr);
        if(bytes_written == table_size)
        {
            std::uint32_t cumulative = table_size;
            bool complete = true;
            for(std::uint32_t block = 0; block < block_count - 1; ++block)
            {
                const std::uint32_t block_size = remaining < 0x8000 ? remaining : 0x8000;
                const std::uint32_t compressed_size = cdf_compressed_write_api.compress(source, block_size, compressed, 0x10000);
                archive->index_size += compressed_size;
                cdf_compressed_write_api.write_file(archive->handle, compressed, compressed_size, &bytes_written, nullptr);
                if(bytes_written != compressed_size)
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
                const DWORD end_position = cdf_compressed_write_api.set_file_pointer(archive->handle, 0, nullptr, FILE_CURRENT);
                cdf_compressed_write_api.set_file_pointer(archive->handle, static_cast<LONG>(table_position), nullptr, FILE_BEGIN);
                cdf_compressed_write_api.write_file(archive->handle, offsets, table_size, &bytes_written, nullptr);
                cdf_compressed_write_api.set_file_pointer(archive->handle, static_cast<LONG>(end_position), nullptr, FILE_BEGIN);
                result = 1;
                archive->error = 0;
            }
        }
    }
    if(compressed != nullptr)
    {
        cdf_compressed_write_api.heap_free(heap, 0, compressed);
    }
    if(offsets != nullptr)
    {
        cdf_compressed_write_api.heap_free(heap, 0, offsets);
    }
    return result;
}

// GAG.EXE: 0x00429B50
std::uint32_t __fastcall write_compressed_cdf_entry(CdfArchive *archive, const void *data)
{
    CdfEntry *entry = archive->entries[archive->write_entry_index];
    std::uint32_t remaining = entry->uncompressed_size;
    const std::uint32_t block_count = (remaining >> 15) + ((remaining & 0x7fff) != 0 ? 1 : 0) + 1;
    HANDLE heap = cdf_compressed_write_api.get_process_heap();
    archive->error = 0x20000;
    std::uint32_t result = 0;
    auto *compressed = static_cast<std::uint8_t *>(cdf_compressed_write_api.heap_alloc(heap, HEAP_ZERO_MEMORY, 0x10000));
    const DWORD table_size = block_count * sizeof(std::uint32_t);
    auto *offsets = static_cast<std::uint32_t *>(cdf_compressed_write_api.heap_alloc(heap, HEAP_ZERO_MEMORY, table_size));
    const auto *source = static_cast<const std::uint8_t *>(data);
    if(source != nullptr && compressed != nullptr && offsets != nullptr)
    {
        offsets[0] = table_size;
        archive->error = 2;
        cdf_compressed_write_api.set_file_pointer(archive->handle, static_cast<LONG>(entry->file_offset), nullptr, FILE_BEGIN);
        DWORD bytes_written;
        cdf_compressed_write_api.write_file(archive->handle, offsets, table_size, &bytes_written, nullptr);
        if(bytes_written == table_size)
        {
            std::uint32_t cumulative = table_size;
            bool complete = true;
            for(std::uint32_t block = 0; block < block_count - 1; ++block)
            {
                const std::uint32_t block_size = remaining < 0x8000 ? remaining : 0x8000;
                const std::uint32_t compressed_size = cdf_compressed_write_api.compress(source, block_size, compressed, 0x10000);
                cdf_compressed_write_api.write_file(archive->handle, compressed, compressed_size, &bytes_written, nullptr);
                if(bytes_written != compressed_size)
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
                const DWORD end_position = cdf_compressed_write_api.set_file_pointer(archive->handle, 0, nullptr, FILE_CURRENT);
                cdf_compressed_write_api.set_file_pointer(archive->handle, static_cast<LONG>(entry->file_offset), nullptr, FILE_BEGIN);
                cdf_compressed_write_api.write_file(archive->handle, offsets, table_size, &bytes_written, nullptr);
                cdf_compressed_write_api.set_file_pointer(archive->handle, static_cast<LONG>(end_position), nullptr, FILE_BEGIN);
                result = 1;
                archive->error = 0;
            }
        }
    }
    if(compressed != nullptr)
    {
        cdf_compressed_write_api.heap_free(heap, 1, compressed);
    }
    if(offsets != nullptr)
    {
        cdf_compressed_write_api.heap_free(heap, 1, offsets);
    }
    return result;
}

// GAG.EXE: 0x004298E0
std::uint32_t __fastcall finalize_cdf_writer(CdfArchive *archive)
{
    if(archive == nullptr)
    {
        return 0;
    }
    archive->entry_count = archive->write_entry_index;
    cdf_writer_finalize_api.write_index(archive);
    cdf_writer_finalize_api.set_file_pointer(archive->handle, 7, nullptr, FILE_BEGIN);
    DWORD bytes_written;
    cdf_writer_finalize_api.write_file(archive->handle, &archive->index_size, 4, &bytes_written, nullptr);
    cdf_writer_finalize_api.write_file(archive->handle, &archive->write_entry_index, 4, &bytes_written, nullptr);
    cdf_writer_finalize_api.write_file(archive->handle, &archive->index_data_size, 4, &bytes_written, nullptr);
    HANDLE heap = cdf_writer_finalize_api.get_process_heap();
    cdf_writer_finalize_api.close_handle(archive->handle);
    cdf_writer_finalize_api.heap_free(heap, 0, archive->entry_storage);
    cdf_writer_finalize_api.heap_free(heap, 0, archive);
    return 0;
}

// GAG.EXE: 0x004297E0
std::uint32_t __fastcall append_cdf_writer_entry(CdfArchive *archive, const char *name, const void *data, std::uint32_t size, int compressed)
{
    if(archive == nullptr || archive->write_entry_index >= archive->entry_count)
    {
        return 0;
    }
    CdfEntry *entry = archive->entries[archive->write_entry_index];
    std::memcpy(entry->name, name, std::strlen(name) + 1);
    entry->flags = cdf_entry_write_api.classify_data(data);
    entry->uncompressed_size = size;
    entry->file_offset = cdf_entry_write_api.set_file_pointer(archive->handle, 0, nullptr, FILE_CURRENT);
    std::uint32_t result;
    if(compressed == 0)
    {
        result = cdf_entry_write_api.write_uncompressed(archive, data);
    }
    else
    {
        entry->flags = static_cast<std::uint8_t>(entry->flags + 0x10);
        result = cdf_entry_write_api.write_compressed(archive, data);
    }
    ++archive->write_entry_index;
    archive->index_data_size += size;
    return result;
}

// GAG.EXE: 0x00429630
CdfArchive *__fastcall create_cdf_writer(const char *path, std::uint32_t capacity)
{
    cdf_last_error = 0x1000000;
    HANDLE file = cdf_writer_create_api.create_file(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if(file == INVALID_HANDLE_VALUE)
    {
        cdf_writer_create_api.get_last_error();
        return nullptr;
    }
    cdf_last_error = 0x20000;
    HANDLE heap = cdf_writer_create_api.get_process_heap();
    auto *archive = static_cast<CdfArchive *>(cdf_writer_create_api.heap_alloc(heap, HEAP_ZERO_MEMORY, sizeof(CdfArchive)));
    if(archive == nullptr)
    {
        cdf_writer_create_api.close_handle(file);
        return nullptr;
    }
    heap = cdf_writer_create_api.get_process_heap();
    archive->entry_storage = cdf_writer_create_api.heap_alloc(heap, HEAP_ZERO_MEMORY, capacity * sizeof(CdfEntry));
    if(archive->entry_storage == nullptr)
    {
        heap = cdf_writer_create_api.get_process_heap();
        cdf_writer_create_api.heap_free(heap, 0, archive);
        cdf_writer_create_api.close_handle(file);
        return nullptr;
    }
    std::memcpy(archive->signature, "CDF97a", 7);
    archive->entry_count = capacity & 0xffff;
    archive->write_entry_index = 0;
    archive->index_data_size = 0;
    archive->entries[0] = static_cast<CdfEntry *>(archive->entry_storage);
    for(std::uint32_t index = 1; index < archive->entry_count; ++index)
    {
        archive->entries[index] = static_cast<CdfEntry *>(archive->entry_storage) + index;
    }
    archive->handle = file;
    DWORD bytes_written;
    cdf_writer_create_api.write_file(file, archive->signature, 7, &bytes_written, nullptr);
    cdf_writer_create_api.write_file(file, &archive->index_size, 4, &bytes_written, nullptr);
    cdf_writer_create_api.write_file(file, &archive->entry_count, 4, &bytes_written, nullptr);
    cdf_writer_create_api.write_file(file, &archive->index_data_size, 4, &bytes_written, nullptr);
    cdf_last_error = 0;
    return archive;
}

// GAG.EXE: 0x00428280
std::uint32_t __fastcall get_cdf_error(CdfArchive *archive)
{
    return archive == nullptr ? cdf_last_error : archive->error;
}

// GAG.EXE: 0x004176A0
std::uint32_t __fastcall write_comment_cdf_package(const char *path, const void *comment, const void *bitmap, const std::uint32_t *configuration)
{
    CdfArchive *archive = cdf_comment_package_api.create_writer(path, 3);
    if(archive == nullptr)
    {
        return 0x20000;
    }
    if(comment != nullptr)
    {
        const std::uint32_t size = static_cast<std::uint32_t>(std::strlen(static_cast<const char *>(comment)) + 1);
        if(cdf_comment_package_api.append_entry(archive, "COMMENT.TXT", comment, size, 0) == 0)
        {
            const std::uint32_t error = cdf_comment_package_api.get_error(archive);
            cdf_comment_package_api.finalize_writer(archive);
            return error;
        }
    }
    if(bitmap != nullptr)
    {
        std::uint16_t bit_count;
        std::memcpy(&bit_count, static_cast<const std::uint8_t *>(bitmap) + 0x1c, sizeof(bit_count));
        if(bit_count == 8)
        {
            std::uint32_t width;
            std::uint32_t height;
            std::memcpy(&width, static_cast<const std::uint8_t *>(bitmap) + 0x12, sizeof(width));
            std::memcpy(&height, static_cast<const std::uint8_t *>(bitmap) + 0x16, sizeof(height));
            if(cdf_comment_package_api.append_entry(archive, "COMMENT.BMP", bitmap, width * height + 0x436, 1) == 0)
            {
                const std::uint32_t error = cdf_comment_package_api.get_error(archive);
                cdf_comment_package_api.finalize_writer(archive);
                return error;
            }
        }
    }
    if(configuration != nullptr)
    {
        const void *data = reinterpret_cast<const void *>(static_cast<std::uintptr_t>(configuration[2]));
        if(cdf_comment_package_api.append_entry(archive, "START.CFG", data, configuration[0], 1) == 0)
        {
            const std::uint32_t error = cdf_comment_package_api.get_error(archive);
            cdf_comment_package_api.finalize_writer(archive);
            return error;
        }
    }
    cdf_comment_package_api.finalize_writer(archive);
    return 0;
}

void set_cdf_io_api_for_testing(const CdfIoApi &api)
{
    cdf_io_api = api;
}

void set_compressed_cdf_reader_for_testing(int(__fastcall *reader)(CdfArchive *, std::uint16_t, void *))
{
    compressed_cdf_reader = reader;
}

void set_cdf_lifecycle_api_for_testing(const CdfLifecycleApi &api)
{
    cdf_lifecycle_api = api;
}

void set_cdf_compression_api_for_testing(const CdfCompressionApi &api)
{
    cdf_compression_api = api;
    compressed_cdf_reader = read_compressed_cdf_entry;
}

void set_cdf_index_api_for_testing(const CdfIndexApi &api)
{
    cdf_index_api = api;
}

void set_cdf_open_api_for_testing(const CdfOpenApi &api)
{
    cdf_open_api = api;
}

void set_cdf_write_api_for_testing(const CdfWriteApi &api)
{
    cdf_write_api = api;
}

void set_cdf_compressed_write_api_for_testing(const CdfCompressedWriteApi &api)
{
    cdf_compressed_write_api = api;
}

void set_cdf_writer_finalize_api_for_testing(const CdfWriterFinalizeApi &api)
{
    cdf_writer_finalize_api = api;
}

void set_cdf_entry_write_api_for_testing(const CdfEntryWriteApi &api)
{
    cdf_entry_write_api = api;
}

void set_cdf_writer_create_api_for_testing(const CdfWriterCreateApi &api)
{
    cdf_writer_create_api = api;
}

void set_cdf_comment_package_api_for_testing(const CdfCommentPackageApi &api)
{
    cdf_comment_package_api = api;
}

std::uint32_t get_cdf_last_error_for_testing()
{
    return cdf_last_error;
}

} // namespace gag
