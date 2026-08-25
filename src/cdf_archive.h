#pragma once

#include <cstddef>
#include <cstdint>

namespace freegag
{
inline constexpr uint8_t CDF_ENTRY_COMPRESSED = 1u << 4;

enum CdfError : uint32_t
{
    CDF_ERROR_NONE = 0,
    CDF_ERROR_READ_FAILED = 1,
    CDF_ERROR_WRITE_FAILED = 2,
    CDF_ERROR_STORAGE_FAILURE = 0x00010000,
    CDF_ERROR_ALLOCATION_FAILED = 0x00020000,
    CDF_ERROR_FILE_OPEN_FAILED = 0x01000000,
    CDF_ERROR_INVALID_FORMAT = 0x02000000
};

struct ScriptTextBuffer;
struct AsyncFileHost;
struct CdfArchive;


// Compression adapters backed by zlib.
int zlib_cdf_decompressor(const void *source, uint32_t source_size, void *destination);
uint32_t zlib_cdf_compressor(const void *source, uint32_t source_size, void *destination, uint32_t destination_capacity);

CdfArchive *open_cdf_archive(const char *path, intptr_t alternate_stream);

const char *get_cdf_entry_name_by_index(CdfArchive *archive, uint32_t index);

uint8_t get_cdf_entry_flags(CdfArchive *archive, const char *name);

uint32_t get_cdf_entry_size(CdfArchive *archive, uint8_t selector, const char *name);

int read_uncompressed_cdf_entry(CdfArchive *archive, uint16_t entry_index, void *destination);

int read_compressed_cdf_entry(CdfArchive *archive, uint16_t entry_index, void *destination);

int read_cdf_entry(CdfArchive *archive, uint8_t selector, const char *name, void *destination);

uint32_t close_cdf_archive(CdfArchive *archive);

int initialize_cdf_index(CdfArchive *archive);

uint32_t write_uncompressed_cdf_entry(CdfArchive *archive, const void *data);

uint32_t write_compressed_cdf_index(CdfArchive *archive);

uint32_t write_compressed_cdf_entry(CdfArchive *archive, const void *data);

uint32_t finalize_cdf_writer(CdfArchive *archive);

uint32_t append_cdf_writer_entry(CdfArchive *archive, const char *name, const void *data, uint32_t size, int compressed);

CdfArchive *create_cdf_writer(const char *path, uint32_t capacity);

uint32_t get_cdf_error(CdfArchive *archive);

void *open_cdf_entry_async_record(CdfArchive *archive, AsyncFileHost *host, const char *name);

uint32_t write_comment_cdf_package(const char *path, const void *comment, const void *bitmap, const ScriptTextBuffer *configuration);


} // namespace freegag
