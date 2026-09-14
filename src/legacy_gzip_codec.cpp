#include "legacy_gzip_codec.h"
#include "legacy_gzip_shim.h"

namespace freegag
{

namespace legacy_gzip_codec
{

std::mutex codec_mutex;

}

LegacyGzipCompressionSession::LegacyGzipCompressionSession()
    : lock_(legacy_gzip_codec::codec_mutex)
{
    freegag_legacy_gzip_begin_compression();
}

uint32_t LegacyGzipCompressionSession::compress(const void *source, uint32_t source_size, void *destination, uint32_t destination_capacity)
{
    return freegag_legacy_gzip_compress(source, source_size, destination, destination_capacity);
}

int legacy_gzip_decompress(const void *source, uint32_t source_size, void *destination, uint32_t destination_capacity, bool accept_full_output)
{
    std::lock_guard lock(legacy_gzip_codec::codec_mutex);
    uint32_t decoded_size;
    if(!freegag_legacy_gzip_decompress(source, source_size, destination, destination_capacity, &decoded_size, accept_full_output))
        return -1;
    return static_cast<int>(decoded_size);
}

}
