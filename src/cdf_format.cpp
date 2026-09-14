#include "cdf_format.h"
#include <cstring>

namespace freegag
{

int decode_cdf_block(const void *source, uint32_t source_size, void *destination, uint32_t destination_capacity)
{
    if(source == nullptr || destination == nullptr || source_size < 2)
        return -1;
    const auto *input = static_cast<const uint8_t *>(source);
    if(input[0] == 0)
    {
        const uint32_t stored_size = source_size - 2;
        if(stored_size > destination_capacity)
            return -1;
        std::memcpy(destination, input + 2, stored_size);
        return static_cast<int>(stored_size);
    }
    if(input[0] != 8)
        return -2;
    return legacy_gzip_decompress(input + 2, source_size - 2, destination, destination_capacity, true);
}

uint32_t encode_cdf_block(LegacyGzipCompressionSession &compression, const void *source, uint32_t source_size, void *destination, uint32_t destination_capacity)
{
    if((source == nullptr && source_size != 0) || destination == nullptr || destination_capacity < source_size + 2)
        return 0;
    auto *output = static_cast<uint8_t *>(destination);
    const uint32_t compressed_size = compression.compress(source, source_size, output + 2, destination_capacity - 2);
    if(compressed_size != 0 && compressed_size < source_size)
    {
        output[0] = 8;
        output[1] = 0;
        return compressed_size + 2;
    }
    output[0] = 0;
    output[1] = 0;
    std::memcpy(output + 2, source, source_size);
    return source_size + 2;
}

}
