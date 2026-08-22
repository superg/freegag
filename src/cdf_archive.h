#pragma once

#include <windows.h>
#include <stddef.h>
#include <stdint.h>

namespace gag
{

struct ScriptTextBuffer;

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
    HANDLE handle;
    intptr_t alternate_stream;
    HANDLE second_handle;
    uint32_t error;
    uint32_t write_entry_index;
    uint32_t index_data_size;
    CdfEntry *entries[2000];
};


struct CdfIoApi
{
    DWORD(WINAPI *set_file_pointer)(HANDLE file, LONG distance, PLONG distance_high, DWORD method);
    BOOL(WINAPI *read_file)(HANDLE file, LPVOID buffer, DWORD bytes_to_read, LPDWORD bytes_read, LPOVERLAPPED overlapped);
    void (*seek_alternate_stream)(HANDLE stream, uint32_t offset);
    void (*read_alternate_stream)(HANDLE stream, void *buffer, uint32_t size, DWORD *bytes_read);
};

struct CdfLifecycleApi
{
    BOOL(WINAPI *close_handle)(HANDLE handle);
    BOOL (*close_alternate_stream)(HANDLE stream);
    HANDLE(WINAPI *get_process_heap)();
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
};

struct CdfCompressionApi
{
    HANDLE(WINAPI *get_process_heap)();
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    int (*decompress)(const void *source, uint32_t compressed_size, void *destination);
};

struct CdfIndexApi
{
    DWORD(WINAPI *get_file_size)(HANDLE file, LPDWORD file_size_high);
    uint32_t (*get_alternate_stream_size)(HANDLE stream);
};

struct CdfOpenApi
{
    HANDLE(WINAPI *create_file)(LPCSTR path, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES security, DWORD creation, DWORD attributes, HANDLE template_file);
    HANDLE (*open_alternate_stream)(intptr_t host, const char *path);
};

struct CdfWriteApi
{
    HANDLE(WINAPI *get_process_heap)();
    DWORD(WINAPI *set_file_pointer)(HANDLE file, LONG distance, PLONG distance_high, DWORD method);
    BOOL(WINAPI *write_file)(HANDLE file, LPCVOID buffer, DWORD bytes_to_write, LPDWORD bytes_written, LPOVERLAPPED overlapped);
};

struct CdfCompressedWriteApi
{
    HANDLE(WINAPI *get_process_heap)();
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    DWORD(WINAPI *set_file_pointer)(HANDLE file, LONG distance, PLONG distance_high, DWORD method);
    BOOL(WINAPI *write_file)(HANDLE file, LPCVOID buffer, DWORD bytes_to_write, LPDWORD bytes_written, LPOVERLAPPED overlapped);
    uint32_t (*compress)(const void *source, uint32_t source_size, void *destination, uint32_t destination_capacity);
};

// Non-original adapters for GAG.EXE's delegated compression-library boundaries.
int zlib_cdf_decompressor(const void *source, uint32_t source_size, void *destination);
uint32_t zlib_cdf_compressor(const void *source, uint32_t source_size, void *destination, uint32_t destination_capacity);

struct CdfWriterFinalizeApi
{
    uint32_t (*write_index)(CdfArchive *archive);
    DWORD(WINAPI *set_file_pointer)(HANDLE file, LONG distance, PLONG distance_high, DWORD method);
    BOOL(WINAPI *write_file)(HANDLE file, LPCVOID buffer, DWORD bytes_to_write, LPDWORD bytes_written, LPOVERLAPPED overlapped);
    HANDLE(WINAPI *get_process_heap)();
    BOOL(WINAPI *close_handle)(HANDLE handle);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
};

struct CdfEntryWriteApi
{
    uint8_t (*classify_data)(const void *data);
    DWORD(WINAPI *set_file_pointer)(HANDLE file, LONG distance, PLONG distance_high, DWORD method);
    uint32_t (*write_uncompressed)(CdfArchive *archive, const void *data);
    uint32_t (*write_compressed)(CdfArchive *archive, const void *data);
};

struct CdfWriterCreateApi
{
    HANDLE(WINAPI *create_file)(LPCSTR path, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES security, DWORD creation, DWORD attributes, HANDLE template_file);
    DWORD(WINAPI *get_last_error)();
    HANDLE(WINAPI *get_process_heap)();
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    BOOL(WINAPI *close_handle)(HANDLE handle);
    BOOL(WINAPI *write_file)(HANDLE file, LPCVOID buffer, DWORD bytes_to_write, LPDWORD bytes_written, LPOVERLAPPED overlapped);
};

struct CdfCommentPackageApi
{
    CdfArchive *(*create_writer)(const char *path, uint32_t capacity);
    uint32_t (*append_entry)(CdfArchive *archive, const char *name, const void *data, uint32_t size, int compressed);
    uint32_t (*get_error)(CdfArchive *archive);
    uint32_t (*finalize_writer)(CdfArchive *archive);
};

// GAG.EXE: 0x004282A0
CdfArchive *open_cdf_archive(const char *path, intptr_t alternate_stream);

// GAG.EXE: 0x00428620
const char *get_cdf_entry_name_by_index(CdfArchive *archive, uint32_t index);

// GAG.EXE: 0x00428690
uint32_t get_cdf_entry_count(CdfArchive *archive);

// GAG.EXE: 0x00428710
uint32_t get_cdf_index_data_size(CdfArchive *archive);

// GAG.EXE: 0x00428630
uint8_t get_cdf_entry_flags(CdfArchive *archive, const char *name);

// GAG.EXE: 0x004286A0
uint32_t get_cdf_entry_size(CdfArchive *archive, uint8_t selector, const char *name);

// GAG.EXE: 0x00429320
int read_uncompressed_cdf_entry(CdfArchive *archive, uint16_t entry_index, void *destination);

// GAG.EXE: 0x004293D0
int read_compressed_cdf_entry(CdfArchive *archive, uint16_t entry_index, void *destination);

// GAG.EXE: 0x004284E0
int read_cdf_entry(CdfArchive *archive, uint8_t selector, const char *name, void *destination);

// GAG.EXE: 0x00428590
uint32_t close_cdf_archive(CdfArchive *archive);

// GAG.EXE: 0x004287E0
int initialize_cdf_index(CdfArchive *archive);

// GAG.EXE: 0x00429A90
uint32_t write_uncompressed_cdf_entry(CdfArchive *archive, const void *data);

// GAG.EXE: 0x00429070
uint32_t write_compressed_cdf_index(CdfArchive *archive);

// GAG.EXE: 0x00429B50
uint32_t write_compressed_cdf_entry(CdfArchive *archive, const void *data);

// GAG.EXE: 0x004298E0
uint32_t finalize_cdf_writer(CdfArchive *archive);

// GAG.EXE: 0x004297E0
uint32_t append_cdf_writer_entry(CdfArchive *archive, const char *name, const void *data, uint32_t size, int compressed);

// GAG.EXE: 0x00429630
CdfArchive *create_cdf_writer(const char *path, uint32_t capacity);

// GAG.EXE: 0x00428280
uint32_t get_cdf_error(CdfArchive *archive);

// GAG.EXE: 0x004176A0
uint32_t write_comment_cdf_package(const char *path, const void *comment, const void *bitmap, const ScriptTextBuffer *configuration);


} // namespace gag
