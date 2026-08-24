#pragma once

#include <cstddef>
#include <cstdint>

namespace gag
{

struct ScriptTextBuffer;
struct AsyncFileHost;
class BinaryInputStream;
class BinaryOutputStream;

struct CdfEntry
{
    uint8_t flags;
    char name[0x23];
    uint32_t file_offset;
    uint32_t uncompressed_size;
};


struct CdfArchive
{
    uint32_t unknown_0000;
    char signature[7];
    uint8_t unknown_000b[9];
    char path[0x104];
    uint32_t index_size;
    void *entry_storage;
    uint32_t entry_count;
    BinaryInputStream *input;
    BinaryOutputStream *output;
    intptr_t alternate_stream;
    BinaryInputStream *second_input;
    uint32_t error;
    uint32_t write_entry_index;
    uint32_t index_data_size;
    CdfEntry *entries[2000];
};


// Compression adapters backed by zlib.
int zlib_cdf_decompressor(const void *source, uint32_t source_size, void *destination);
uint32_t zlib_cdf_compressor(const void *source, uint32_t source_size, void *destination, uint32_t destination_capacity);

CdfArchive *open_cdf_archive(const char *path, intptr_t alternate_stream);

const char *get_cdf_entry_name_by_index(CdfArchive *archive, uint32_t index);

uint32_t get_cdf_entry_count(CdfArchive *archive);

uint32_t get_cdf_index_data_size(CdfArchive *archive);

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

void *open_cdf_entry_async_record(CdfArchive *archive, AsyncFileHost *host, uint32_t start, uint32_t end);

uint32_t write_comment_cdf_package(const char *path, const void *comment, const void *bitmap, const ScriptTextBuffer *configuration);


} // namespace gag
