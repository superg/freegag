#include "asset_decoders.h"
#include <algorithm>
#include <cstddef>
#include <limits>

namespace xtet
{
namespace
{

bool span_fits(std::size_t offset, std::size_t size, std::size_t total)
{
    return offset <= total && size <= total - offset;
}

std::uint16_t read_u16(const std::uint8_t *data)
{
    return (std::uint16_t)((std::uint16_t)data[0] | (std::uint16_t)((std::uint16_t)data[1] << 8));
}

std::uint32_t read_u32(const std::uint8_t *data)
{
    return (std::uint32_t)data[0] | ((std::uint32_t)data[1] << 8) | ((std::uint32_t)data[2] << 16) | ((std::uint32_t)data[3] << 24);
}

bool is_fourcc(const std::uint8_t *data, const char *value)
{
    return std::equal(data, data + 4, (const std::uint8_t *)value);
}

} // namespace

bool decode_indexed_bitmap(const std::vector<std::uint8_t> &bytes, IndexedBitmap &bitmap)
{
    if(bytes.size() < 54 || bytes[0] != 'B' || bytes[1] != 'M')
        return false;
    const std::size_t pixel_offset = read_u32(bytes.data() + 10);
    const std::uint32_t dib_size = read_u32(bytes.data() + 14);
    if(dib_size < 40 || !span_fits(14, dib_size, bytes.size()))
        return false;
    const std::int32_t signed_width = (std::int32_t)read_u32(bytes.data() + 18);
    const std::int32_t signed_height = (std::int32_t)read_u32(bytes.data() + 22);
    const std::uint16_t planes = read_u16(bytes.data() + 26);
    const std::uint16_t bits_per_pixel = read_u16(bytes.data() + 28);
    const std::uint32_t compression = read_u32(bytes.data() + 30);
    const std::uint32_t used_colors = read_u32(bytes.data() + 46);
    if(signed_width <= 0 || signed_height == 0 || signed_height == std::numeric_limits<std::int32_t>::min() || planes != 1 || bits_per_pixel != 8 || compression != 0 || used_colors > 256)
        return false;

    const std::uint32_t width = (std::uint32_t)signed_width;
    const std::uint32_t height = (std::uint32_t)(signed_height < 0 ? -signed_height : signed_height);
    const std::size_t palette_count = used_colors ? used_colors : 256;
    const std::size_t palette_offset = 14 + dib_size;
    if(!span_fits(palette_offset, palette_count * 4, bytes.size()) || pixel_offset < palette_offset + palette_count * 4)
        return false;
    const std::uint64_t row_stride_64 = ((std::uint64_t)width + 3) & ~(std::uint64_t)3;
    const std::uint64_t pixel_count_64 = (std::uint64_t)width * height;
    const std::uint64_t stored_size_64 = row_stride_64 * height;
    if(row_stride_64 > std::numeric_limits<std::size_t>::max() || pixel_count_64 > std::numeric_limits<std::size_t>::max() || stored_size_64 > std::numeric_limits<std::size_t>::max()
        || !span_fits(pixel_offset, (std::size_t)stored_size_64, bytes.size()))
        return false;

    IndexedBitmap decoded;
    decoded.width = width;
    decoded.height = height;
    for(std::size_t index = 0; index < palette_count; ++index)
    {
        const std::uint8_t *color = bytes.data() + palette_offset + index * 4;
        decoded.palette[index] = { color[0], color[1], color[2], color[3] };
    }
    decoded.pixels.resize((std::size_t)pixel_count_64);
    const std::size_t row_stride = (std::size_t)row_stride_64;
    for(std::uint32_t y = 0; y < height; ++y)
    {
        const std::uint32_t source_y = signed_height < 0 ? y : height - y - 1;
        std::copy_n(bytes.data() + pixel_offset + (std::size_t)source_y * row_stride, width, decoded.pixels.data() + (std::size_t)y * width);
    }
    bitmap = std::move(decoded);
    return true;
}

bool decode_wave_pcm(const std::vector<std::uint8_t> &bytes, WavePcm &wave)
{
    if(bytes.size() < 12 || !is_fourcc(bytes.data(), "RIFF") || !is_fourcc(bytes.data() + 8, "WAVE"))
        return false;
    const std::uint32_t riff_size = read_u32(bytes.data() + 4);
    if(riff_size < 4 || (std::uint64_t)riff_size + 8 > bytes.size())
        return false;

    PcmFormat format{};
    const std::uint8_t *sample_data = nullptr;
    std::size_t sample_size = 0;
    bool has_format = false;
    std::size_t offset = 12;
    const std::size_t riff_end = (std::size_t)riff_size + 8;
    while(offset + 8 <= riff_end)
    {
        const std::uint8_t *chunk = bytes.data() + offset;
        const std::uint32_t chunk_size = read_u32(chunk + 4);
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
        || format.average_bytes_per_second != (std::uint64_t)format.samples_per_second * format.block_alignment || format.bits_per_sample == 0 || sample_size % format.block_alignment != 0)
        return false;
    WavePcm decoded;
    decoded.format = format;
    decoded.samples.assign(sample_data, sample_data + sample_size);
    wave = std::move(decoded);
    return true;
}

} // namespace xtet
