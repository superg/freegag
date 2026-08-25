#include "asset_decoders.h"
#include <algorithm>
#include <limits>
#include <stddef.h>

namespace xtet
{

bool span_fits(size_t offset, size_t size, size_t total)
{
    return offset <= total && size <= total - offset;
}

uint16_t read_u16(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | (uint16_t)((uint16_t)data[1] << 8));
}

uint32_t read_u32(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8) | ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

bool is_fourcc(const uint8_t *data, const char *value)
{
    return std::equal(data, data + 4, (const uint8_t *)value);
}


bool decode_indexed_bitmap(const std::vector<uint8_t> &bytes, IndexedBitmap &bitmap)
{
    if(bytes.size() < 54 || bytes[0] != 'B' || bytes[1] != 'M')
        return false;
    const size_t pixel_offset = read_u32(bytes.data() + 10);
    const uint32_t dib_size = read_u32(bytes.data() + 14);
    if(dib_size < 40 || !span_fits(14, dib_size, bytes.size()))
        return false;
    const int32_t signed_width = (int32_t)read_u32(bytes.data() + 18);
    const int32_t signed_height = (int32_t)read_u32(bytes.data() + 22);
    const uint16_t planes = read_u16(bytes.data() + 26);
    const uint16_t bits_per_pixel = read_u16(bytes.data() + 28);
    const uint32_t compression = read_u32(bytes.data() + 30);
    const uint32_t used_colors = read_u32(bytes.data() + 46);
    if(signed_width <= 0 || signed_height == 0 || signed_height == std::numeric_limits<int32_t>::min() || planes != 1 || bits_per_pixel != 8 || compression != 0 || used_colors > 256)
        return false;

    const uint32_t width = (uint32_t)signed_width;
    const uint32_t height = (uint32_t)(signed_height < 0 ? -signed_height : signed_height);
    const size_t palette_count = used_colors ? used_colors : 256;
    const size_t palette_offset = 14 + dib_size;
    if(!span_fits(palette_offset, palette_count * 4, bytes.size()) || pixel_offset < palette_offset + palette_count * 4)
        return false;
    const uint64_t row_stride_64 = ((uint64_t)width + 3) & ~(uint64_t)3;
    const uint64_t pixel_count_64 = (uint64_t)width * height;
    const uint64_t stored_size_64 = row_stride_64 * height;
    if(row_stride_64 > std::numeric_limits<size_t>::max() || pixel_count_64 > std::numeric_limits<size_t>::max() || stored_size_64 > std::numeric_limits<size_t>::max()
        || !span_fits(pixel_offset, (size_t)stored_size_64, bytes.size()))
        return false;

    IndexedBitmap decoded;
    decoded.width = width;
    decoded.height = height;
    for(size_t index = 0; index < palette_count; ++index)
    {
        const uint8_t *color = bytes.data() + palette_offset + index * 4;
        decoded.palette[index] = { color[0], color[1], color[2] };
    }
    decoded.pixels.resize((size_t)pixel_count_64);
    const size_t row_stride = (size_t)row_stride_64;
    for(uint32_t y = 0; y < height; ++y)
    {
        const uint32_t source_y = signed_height < 0 ? y : height - y - 1;
        std::copy_n(bytes.data() + pixel_offset + (size_t)source_y * row_stride, width, decoded.pixels.data() + (size_t)y * width);
    }
    bitmap = std::move(decoded);
    return true;
}

bool decode_wave_pcm(const std::vector<uint8_t> &bytes, WavePcm &wave)
{
    if(bytes.size() < 12 || !is_fourcc(bytes.data(), "RIFF") || !is_fourcc(bytes.data() + 8, "WAVE"))
        return false;
    const uint32_t riff_size = read_u32(bytes.data() + 4);
    if(riff_size < 4 || (uint64_t)riff_size + 8 > bytes.size())
        return false;

    PcmFormat format{};
    const uint8_t *sample_data = nullptr;
    size_t sample_size = 0;
    bool has_format = false;
    size_t offset = 12;
    const size_t riff_end = (size_t)riff_size + 8;
    while(offset + 8 <= riff_end)
    {
        const uint8_t *chunk = bytes.data() + offset;
        const uint32_t chunk_size = read_u32(chunk + 4);
        offset += 8;
        if(!span_fits(offset, chunk_size, riff_end))
            return false;
        if(is_fourcc(chunk, "fmt "))
        {
            if(has_format || chunk_size < sizeof(PcmFormat))
                return false;
            format.format_tag = read_u16(bytes.data() + offset);
            format.channel_count = read_u16(bytes.data() + offset + 2);
            format.samples_per_second = read_u32(bytes.data() + offset + 4);
            format.average_bytes_per_second = read_u32(bytes.data() + offset + 8);
            format.block_alignment = read_u16(bytes.data() + offset + 12);
            format.bits_per_sample = read_u16(bytes.data() + offset + 14);
            has_format = true;
        }
        else if(is_fourcc(chunk, "data"))
        {
            if(!has_format || sample_data)
                return false;
            sample_data = bytes.data() + offset;
            sample_size = chunk_size;
            break;
        }
        offset += chunk_size;
        if((chunk_size & 1) != 0 && offset < riff_end)
            ++offset;
    }
    if(!has_format || !sample_data || format.format_tag != 1 || format.channel_count == 0 || format.samples_per_second == 0 || format.block_alignment == 0
        || format.average_bytes_per_second != (uint64_t)format.samples_per_second * format.block_alignment || format.bits_per_sample == 0 || sample_size % format.block_alignment != 0)
        return false;
    WavePcm decoded;
    decoded.format = format;
    decoded.samples.assign(sample_data, sample_data + sample_size);
    wave = std::move(decoded);
    return true;
}

} // namespace xtet
