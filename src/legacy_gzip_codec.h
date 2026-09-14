#pragma once

#include <cstdint>
#include <mutex>



namespace freegag
{

class LegacyGzipCompressionSession
{
public:
    LegacyGzipCompressionSession();

    LegacyGzipCompressionSession(const LegacyGzipCompressionSession &) = delete;
    LegacyGzipCompressionSession &operator=(const LegacyGzipCompressionSession &) = delete;

    uint32_t compress(const void *source, uint32_t source_size, void *destination, uint32_t destination_capacity);

private:
    std::unique_lock<std::mutex> lock_;
};

int legacy_gzip_decompress(const void *source, uint32_t source_size, void *destination, uint32_t destination_capacity, bool accept_full_output = false);

}
