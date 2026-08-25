#include "rli_animation.h"
#include <algorithm>
#include <limits>

namespace xtet
{

constexpr size_t rli_declared_size_offset = 2;
constexpr size_t rli_frame_count_offset = 6;
constexpr size_t rli_width_offset = 0x16;
constexpr size_t rli_height_offset = 0x1a;
constexpr size_t rli_frame_records_offset = 0x53a;
constexpr size_t rli_frame_record_size = 20;
constexpr uint16_t rli_signature = 0x054e;

uint16_t read_u16(const std::vector<uint8_t> &bytes, size_t offset)
{
    return (uint16_t)(bytes[offset] | ((uint16_t)bytes[offset + 1] << 8));
}

uint32_t read_u32(const std::vector<uint8_t> &bytes, size_t offset)
{
    return (uint32_t)bytes[offset] | ((uint32_t)bytes[offset + 1] << 8) | ((uint32_t)bytes[offset + 2] << 16) | ((uint32_t)bytes[offset + 3] << 24);
}

bool decode_rle8(const uint8_t *source, size_t source_size, uint32_t width, uint32_t height, std::vector<uint8_t> &pixels, std::vector<uint8_t> &coverage)
{
    pixels.assign((size_t)width * height, 0);
    coverage.assign(pixels.size(), 0);
    size_t offset = 0;
    int32_t x = 0;
    int32_t y = (int32_t)height - 1;
    while(offset + 2 <= source_size)
    {
        const uint8_t count = source[offset++];
        const uint8_t value = source[offset++];
        if(count != 0)
        {
            if(y < 0 || x < 0 || (uint32_t)x + count > width)
                return false;
            std::fill_n(pixels.begin() + (size_t)y * width + x, count, value);
            std::fill_n(coverage.begin() + (size_t)y * width + x, count, (uint8_t)1);
            x += count;
        }
        else if(value == 0)
        {
            x = 0;
            --y;
        }
        else if(value == 1)
        {
            return true;
        }
        else if(value == 2)
        {
            if(offset + 2 > source_size)
                return false;
            x += source[offset++];
            y -= source[offset++];
            if(x < 0 || (uint32_t)x > width || y < -1 || y >= (int32_t)height)
                return false;
        }
        else
        {
            if(offset + value > source_size || y < 0 || x < 0 || (uint32_t)x + value > width)
                return false;
            std::copy_n(source + offset, value, pixels.begin() + (size_t)y * width + x);
            std::fill_n(coverage.begin() + (size_t)y * width + x, value, (uint8_t)1);
            offset += value;
            x += value;
            if((value & 1) != 0)
            {
                if(offset == source_size)
                    return false;
                ++offset;
            }
        }
    }
    return false;
}


bool decode_rli_animation(const std::string &path, const std::vector<uint8_t> &bytes, RliAnimation &animation)
{
    animation = {};
    if(bytes.size() < rli_frame_records_offset + rli_frame_record_size || read_u16(bytes, 0) != rli_signature)
        return false;
    const uint32_t declared_size = read_u32(bytes, rli_declared_size_offset);
    const uint16_t frame_count = read_u16(bytes, rli_frame_count_offset);
    const size_t record_count = (size_t)frame_count + 1;
    if(record_count > ((std::numeric_limits<size_t>::max)() - rli_frame_records_offset) / rli_frame_record_size)
        return false;
    const size_t records_end = rli_frame_records_offset + record_count * rli_frame_record_size;
    if(declared_size < records_end || declared_size > bytes.size())
        return false;

    animation.path = path;
    animation.width = (int32_t)read_u32(bytes, rli_width_offset);
    animation.height = (int32_t)read_u32(bytes, rli_height_offset);
    if(animation.width <= 0 || animation.height <= 0)
    {
        animation = {};
        return false;
    }
    animation.resident_bytes.assign(bytes.begin(), bytes.begin() + declared_size);
    animation.frame_records.resize(record_count);
    std::array<PaletteColor, 256> current_palette{};
    std::array<uint8_t, 256> current_palette_defined{};
    for(size_t index = 0; index < record_count; ++index)
    {
        const size_t record_offset = rli_frame_records_offset + index * rli_frame_record_size;
        const auto source = bytes.begin() + record_offset;
        std::copy_n(source, rli_frame_record_size, animation.frame_records[index].bytes.begin());
        RliFrameRecord &record = animation.frame_records[index];
        record.flags = read_u16(bytes, record_offset);
        record.data_offset = read_u32(bytes, record_offset + 4);
        record.palette_start = (int16_t)read_u16(bytes, record_offset + 8);
        record.palette_count = (int16_t)read_u16(bytes, record_offset + 10);
        record.left = (int16_t)read_u16(bytes, record_offset + 12);
        record.top = (int16_t)read_u16(bytes, record_offset + 14);
        record.right = (int16_t)read_u16(bytes, record_offset + 16);
        record.bottom = (int16_t)read_u16(bytes, record_offset + 18);
        if(record.data_offset > declared_size || (index == 0 && record.data_offset < records_end) || (index != 0 && record.data_offset < animation.frame_records[index - 1].data_offset))
        {
            animation = {};
            return false;
        }
    }
    for(size_t index = 0; index < record_count; ++index)
    {
        RliFrameRecord &record = animation.frame_records[index];
        const size_t data_end = index + 1 < record_count ? animation.frame_records[index + 1].data_offset : declared_size;
        size_t data_offset = record.data_offset;
        if(record.right < record.left || record.bottom < record.top || record.left < 0 || record.top < 0 || record.right >= animation.width || record.bottom >= animation.height)
            return false;
        if((record.flags & RLI_FRAME_PALETTE) != 0)
        {
            if(record.palette_start < 0 || record.palette_count < 0 || (uint32_t)record.palette_start + record.palette_count > 256)
                return false;
            const size_t palette_size = (size_t)record.palette_count * 4;
            if(palette_size > data_end - data_offset)
                return false;
            for(int32_t palette_index = 0; palette_index < record.palette_count; ++palette_index)
            {
                const size_t source_offset = data_offset + static_cast<size_t>(palette_index) * 4;
                const size_t destination_index = static_cast<size_t>(record.palette_start + palette_index);
                current_palette[destination_index] = { bytes[source_offset], bytes[source_offset + 1], bytes[source_offset + 2] };
                current_palette_defined[destination_index] = 1;
            }
            data_offset += palette_size;
        }
        record.palette = current_palette;
        record.palette_defined = current_palette_defined;
        const uint32_t width = (uint32_t)(record.right - record.left + 1);
        const uint32_t height = (uint32_t)(record.bottom - record.top + 1);
        if((record.flags & RLI_FRAME_RAW_PIXELS) != 0)
        {
            const size_t source_stride = (width + 3) & ~(size_t)3;
            if(source_stride > (data_end - data_offset) / height)
                return false;
            record.pixels.resize((size_t)width * height);
            record.coverage.assign(record.pixels.size(), 1);
            for(uint32_t row = 0; row < height; ++row)
                std::copy_n(bytes.data() + data_offset + (size_t)(height - row - 1) * source_stride, width, record.pixels.begin() + (size_t)row * width);
        }
        else if((record.flags & RLI_FRAME_RLE_PIXELS) != 0)
        {
            if(!decode_rle8(bytes.data() + data_offset, data_end - data_offset, width, height, record.pixels, record.coverage))
                return false;
        }
    }
    return true;
}

bool load_rli_animations(const SfsArchive &archive, const std::vector<std::string> &paths, std::vector<RliAnimation> &animations)
{
    animations.clear();
    animations.reserve(paths.size());
    for(const std::string &path : paths)
    {
        std::vector<uint8_t> bytes;
        RliAnimation animation;
        if(!archive.read(path, bytes) || !decode_rli_animation(path, bytes, animation))
        {
            animations.clear();
            return false;
        }
        animations.push_back(std::move(animation));
    }
    return true;
}

} // namespace xtet
