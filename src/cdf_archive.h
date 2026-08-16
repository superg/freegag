#pragma once

#include <windows.h>
#include <cstddef>
#include <cstdint>

namespace gag
{

struct CdfEntry
{
    std::uint8_t flags;
    char name[0x23];
    std::uint32_t file_offset;
    std::uint32_t uncompressed_size;
};

static_assert(sizeof(CdfEntry) == 0x2c);
static_assert(offsetof(CdfEntry, file_offset) == 0x24);
static_assert(offsetof(CdfEntry, uncompressed_size) == 0x28);

struct CdfArchive
{
    std::uint32_t unknown_0000;
    char signature[7];
    std::uint8_t unknown_000b[9];
    char path[0x104];
    std::uint32_t index_size;
    void *entry_storage;
    std::uint32_t entry_count;
    HANDLE handle;
    std::uint32_t alternate_stream;
    HANDLE second_handle;
    std::uint32_t error;
    std::uint32_t write_entry_index;
    std::uint32_t index_data_size;
    CdfEntry *entries[2000];
};

static_assert(sizeof(CdfArchive) == 0x207c);
static_assert(offsetof(CdfArchive, signature) == 4);
static_assert(offsetof(CdfArchive, path) == 0x14);
static_assert(offsetof(CdfArchive, index_size) == 0x118);
static_assert(offsetof(CdfArchive, entry_storage) == 0x11c);
static_assert(offsetof(CdfArchive, entry_count) == 0x120);
static_assert(offsetof(CdfArchive, handle) == 0x124);
static_assert(offsetof(CdfArchive, alternate_stream) == 0x128);
static_assert(offsetof(CdfArchive, second_handle) == 0x12c);
static_assert(offsetof(CdfArchive, error) == 0x130);
static_assert(offsetof(CdfArchive, write_entry_index) == 0x134);
static_assert(offsetof(CdfArchive, index_data_size) == 0x138);
static_assert(offsetof(CdfArchive, entries) == 0x13c);

struct CdfIoApi
{
    DWORD(WINAPI *set_file_pointer)(HANDLE file, LONG distance, PLONG distance_high, DWORD method);
    BOOL(WINAPI *read_file)(HANDLE file, LPVOID buffer, DWORD bytes_to_read, LPDWORD bytes_read, LPOVERLAPPED overlapped);
    void(__fastcall *seek_alternate_stream)(HANDLE stream, std::uint32_t offset);
    void(__fastcall *read_alternate_stream)(HANDLE stream, void *buffer, std::uint32_t size, DWORD *bytes_read);
};

struct CdfLifecycleApi
{
    BOOL(WINAPI *close_handle)(HANDLE handle);
    BOOL(__fastcall *close_alternate_stream)(HANDLE stream);
    HANDLE(WINAPI *get_process_heap)();
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
};

struct CdfCompressionApi
{
    HANDLE(WINAPI *get_process_heap)();
    LPVOID(WINAPI *heap_alloc)(HANDLE heap, DWORD flags, SIZE_T bytes);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
    int(__fastcall *decompress)(const void *source, std::uint32_t compressed_size, void *destination);
};

struct CdfIndexApi
{
    DWORD(WINAPI *get_file_size)(HANDLE file, LPDWORD file_size_high);
    std::uint32_t(__fastcall *get_alternate_stream_size)(HANDLE stream);
};

struct CdfOpenApi
{
    HANDLE(WINAPI *create_file)(LPCSTR path, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES security, DWORD creation, DWORD attributes, HANDLE template_file);
    HANDLE(__fastcall *open_alternate_stream)(const char *path);
};

struct CdfWriteApi
{
    HANDLE(WINAPI *get_process_heap)();
    DWORD(WINAPI *set_file_pointer)(HANDLE file, LONG distance, PLONG distance_high, DWORD method);
    BOOL(WINAPI *write_file)(HANDLE file, LPCVOID buffer, DWORD bytes_to_write, LPDWORD bytes_written, LPOVERLAPPED overlapped);
};

struct CdfWriterFinalizeApi
{
    std::uint32_t(__fastcall *write_index)(CdfArchive *archive);
    DWORD(WINAPI *set_file_pointer)(HANDLE file, LONG distance, PLONG distance_high, DWORD method);
    BOOL(WINAPI *write_file)(HANDLE file, LPCVOID buffer, DWORD bytes_to_write, LPDWORD bytes_written, LPOVERLAPPED overlapped);
    HANDLE(WINAPI *get_process_heap)();
    BOOL(WINAPI *close_handle)(HANDLE handle);
    BOOL(WINAPI *heap_free)(HANDLE heap, DWORD flags, LPVOID memory);
};

struct CdfEntryWriteApi
{
    std::uint8_t(__fastcall *classify_data)(const void *data);
    DWORD(WINAPI *set_file_pointer)(HANDLE file, LONG distance, PLONG distance_high, DWORD method);
    std::uint32_t(__fastcall *write_uncompressed)(CdfArchive *archive, const void *data);
    std::uint32_t(__fastcall *write_compressed)(CdfArchive *archive, const void *data);
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
    CdfArchive *(__fastcall *create_writer)(const char *path, std::uint32_t capacity);
    std::uint32_t(__fastcall *append_entry)(CdfArchive *archive, const char *name, const void *data, std::uint32_t size, int compressed);
    std::uint32_t(__fastcall *get_error)(CdfArchive *archive);
    std::uint32_t(__fastcall *finalize_writer)(CdfArchive *archive);
};

// GAG.EXE: 0x004282A0
CdfArchive *__fastcall open_cdf_archive(const char *path, int alternate_stream);

// GAG.EXE: 0x00428620
const char *__fastcall get_cdf_entry_name_by_index(CdfArchive *archive, std::uint32_t index);

// GAG.EXE: 0x00428690
std::uint32_t __fastcall get_cdf_entry_count(CdfArchive *archive);

// GAG.EXE: 0x00428710
std::uint32_t __fastcall get_cdf_index_data_size(CdfArchive *archive);

// GAG.EXE: 0x00428630
std::uint8_t __fastcall get_cdf_entry_flags(CdfArchive *archive, const char *name);

// GAG.EXE: 0x004286A0
std::uint32_t __fastcall get_cdf_entry_size(CdfArchive *archive, std::uint8_t selector, const char *name);

// GAG.EXE: 0x00429320
int __fastcall read_uncompressed_cdf_entry(CdfArchive *archive, std::uint16_t entry_index, void *destination);

// GAG.EXE: 0x004293D0
int __fastcall read_compressed_cdf_entry(CdfArchive *archive, std::uint16_t entry_index, void *destination);

// GAG.EXE: 0x004284E0
int __fastcall read_cdf_entry(CdfArchive *archive, std::uint8_t selector, const char *name, void *destination);

// GAG.EXE: 0x00428590
std::uint32_t __fastcall close_cdf_archive(CdfArchive *archive);

// GAG.EXE: 0x004287E0
int __fastcall initialize_cdf_index(CdfArchive *archive);

// GAG.EXE: 0x00429A90
std::uint32_t __fastcall write_uncompressed_cdf_entry(CdfArchive *archive, const void *data);

// GAG.EXE: 0x004298E0
std::uint32_t __fastcall finalize_cdf_writer(CdfArchive *archive);

// GAG.EXE: 0x004297E0
std::uint32_t __fastcall append_cdf_writer_entry(CdfArchive *archive, const char *name, const void *data, std::uint32_t size, int compressed);

// GAG.EXE: 0x00429630
CdfArchive *__fastcall create_cdf_writer(const char *path, std::uint32_t capacity);

// GAG.EXE: 0x00428280
std::uint32_t __fastcall get_cdf_error(CdfArchive *archive);

// GAG.EXE: 0x004176A0
std::uint32_t __fastcall write_comment_cdf_package(const char *path, const void *comment, const void *bitmap, const std::uint32_t *configuration);

void set_cdf_io_api_for_testing(const CdfIoApi &api);
void set_compressed_cdf_reader_for_testing(int(__fastcall *reader)(CdfArchive *, std::uint16_t, void *));
void set_cdf_lifecycle_api_for_testing(const CdfLifecycleApi &api);
void set_cdf_compression_api_for_testing(const CdfCompressionApi &api);
void set_cdf_index_api_for_testing(const CdfIndexApi &api);
void set_cdf_open_api_for_testing(const CdfOpenApi &api);
void set_cdf_write_api_for_testing(const CdfWriteApi &api);
void set_cdf_writer_finalize_api_for_testing(const CdfWriterFinalizeApi &api);
void set_cdf_entry_write_api_for_testing(const CdfEntryWriteApi &api);
void set_cdf_writer_create_api_for_testing(const CdfWriterCreateApi &api);
void set_cdf_comment_package_api_for_testing(const CdfCommentPackageApi &api);
std::uint32_t get_cdf_last_error_for_testing();

} // namespace gag
