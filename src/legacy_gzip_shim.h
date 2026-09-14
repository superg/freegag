#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void freegag_legacy_gzip_begin_compression(void);

    uint32_t freegag_legacy_gzip_compress(const void *source, uint32_t source_size, void *destination, uint32_t destination_capacity);

    int freegag_legacy_gzip_decompress(const void *source, uint32_t source_size, void *destination, uint32_t destination_capacity, uint32_t *decoded_size, int accept_full_output);

#ifdef __cplusplus
}
#endif
