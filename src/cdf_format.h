#pragma once

#include <cstdint>
#include "legacy_gzip_codec.h"



namespace freegag
{

inline constexpr char CDF_97A_SIGNATURE[7]{ 'C', 'D', 'F', '9', '7', 'a', '\0' };
inline constexpr uint32_t CDF_BLOCK_SIZE = 0x8000;
inline constexpr uint8_t CDF_ENTRY_COMPRESSED = 1u << 4;

struct CdfEntry
{
    uint8_t flags;
    char name[0x23];
    uint32_t file_offset;
    uint32_t uncompressed_size;
};

static_assert(sizeof(CdfEntry) == 44);

int decode_cdf_block(const void *source, uint32_t source_size, void *destination, uint32_t destination_capacity);

uint32_t encode_cdf_block(LegacyGzipCompressionSession &compression, const void *source, uint32_t source_size, void *destination, uint32_t destination_capacity);

}
