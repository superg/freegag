#include "rli_animation.h"
#include <algorithm>
#include <limits>

namespace xtet
{
namespace
{

constexpr std::size_t rli_declared_size_offset = 2;
constexpr std::size_t rli_frame_count_offset = 6;
constexpr std::size_t rli_flags_offset = 8;
constexpr std::size_t rli_width_offset = 0x16;
constexpr std::size_t rli_height_offset = 0x1a;
constexpr std::size_t rli_frame_records_offset = 0x53a;
constexpr std::size_t rli_frame_record_size = 20;
constexpr std::uint16_t rli_signature = 0x054e;

std::uint16_t read_u16(const std::vector<std::uint8_t> &bytes, std::size_t offset)
{
    return (std::uint16_t)(bytes[offset] | ((std::uint16_t)bytes[offset + 1] << 8));
}

std::uint32_t read_u32(const std::vector<std::uint8_t> &bytes, std::size_t offset)
{
    return (std::uint32_t)bytes[offset] | ((std::uint32_t)bytes[offset + 1] << 8) | ((std::uint32_t)bytes[offset + 2] << 16) | ((std::uint32_t)bytes[offset + 3] << 24);
}

bool decode_rle8(const std::uint8_t *source, std::size_t source_size, std::uint32_t width, std::uint32_t height, std::vector<std::uint8_t> &pixels, std::vector<std::uint8_t> &coverage)
{
    pixels.assign((std::size_t)width * height, 0);
    coverage.assign(pixels.size(), 0);
    std::size_t offset = 0;
    std::int32_t x = 0;
    std::int32_t y = (std::int32_t)height - 1;
    while(offset + 2 <= source_size)
    {
        const std::uint8_t count = source[offset++];
        const std::uint8_t value = source[offset++];
        if(count != 0)
        {
            if(y < 0 || x < 0 || (std::uint32_t)x + count > width)
                return false;
            std::fill_n(pixels.begin() + (std::size_t)y * width + x, count, value);
            std::fill_n(coverage.begin() + (std::size_t)y * width + x, count, (std::uint8_t)1);
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
            if(x < 0 || (std::uint32_t)x > width || y < -1 || y >= (std::int32_t)height)
                return false;
        }
        else
        {
            if(offset + value > source_size || y < 0 || x < 0 || (std::uint32_t)x + value > width)
                return false;
            std::copy_n(source + offset, value, pixels.begin() + (std::size_t)y * width + x);
            std::fill_n(coverage.begin() + (std::size_t)y * width + x, value, (std::uint8_t)1);
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

} // namespace

bool decode_rli_animation(const std::string &path, const std::vector<std::uint8_t> &bytes, RliAnimation &animation)
{
    animation = {};
    if(bytes.size() < rli_frame_records_offset + rli_frame_record_size || read_u16(bytes, 0) != rli_signature)
        return false;
    const std::uint32_t declared_size = read_u32(bytes, rli_declared_size_offset);
    const std::uint16_t frame_count = read_u16(bytes, rli_frame_count_offset);
    const std::size_t record_count = (std::size_t)frame_count + 1;
    if(record_count > ((std::numeric_limits<std::size_t>::max)() - rli_frame_records_offset) / rli_frame_record_size)
        return false;
    const std::size_t records_end = rli_frame_records_offset + record_count * rli_frame_record_size;
    if(declared_size < records_end || declared_size > bytes.size())
        return false;

    animation.path = path;
    animation.flags = read_u16(bytes, rli_flags_offset) & 0x1000;
    animation.width = (std::int32_t)read_u32(bytes, rli_width_offset);
    animation.height = (std::int32_t)read_u32(bytes, rli_height_offset);
    if(animation.width <= 0 || animation.height <= 0)
    {
        animation = {};
        return false;
    }
    animation.resident_bytes.assign(bytes.begin(), bytes.begin() + declared_size);
    animation.frame_records.resize(record_count);
    for(std::size_t index = 0; index < record_count; ++index)
    {
        const std::size_t record_offset = rli_frame_records_offset + index * rli_frame_record_size;
        const auto source = bytes.begin() + record_offset;
        std::copy_n(source, rli_frame_record_size, animation.frame_records[index].bytes.begin());
        RliFrameRecord &record = animation.frame_records[index];
        record.flags = read_u16(bytes, record_offset);
        record.data_offset = read_u32(bytes, record_offset + 4);
        record.palette_start = (std::int16_t)read_u16(bytes, record_offset + 8);
        record.palette_count = (std::int16_t)read_u16(bytes, record_offset + 10);
        record.left = (std::int16_t)read_u16(bytes, record_offset + 12);
        record.top = (std::int16_t)read_u16(bytes, record_offset + 14);
        record.right = (std::int16_t)read_u16(bytes, record_offset + 16);
        record.bottom = (std::int16_t)read_u16(bytes, record_offset + 18);
        if(record.data_offset > declared_size || (index == 0 && record.data_offset < records_end) || (index != 0 && record.data_offset < animation.frame_records[index - 1].data_offset))
        {
            animation = {};
            return false;
        }
    }
    for(std::size_t index = 0; index < record_count; ++index)
    {
        RliFrameRecord &record = animation.frame_records[index];
        const std::size_t data_end = index + 1 < record_count ? animation.frame_records[index + 1].data_offset : declared_size;
        std::size_t data_offset = record.data_offset;
        if(record.right < record.left || record.bottom < record.top || record.left < 0 || record.top < 0 || record.right >= animation.width || record.bottom >= animation.height)
            return false;
        if((record.flags & 4) != 0)
        {
            if(record.palette_start < 0 || record.palette_count < 0 || (std::uint32_t)record.palette_start + record.palette_count > 256)
                return false;
            const std::size_t palette_size = (std::size_t)record.palette_count * 4;
            if(palette_size > data_end - data_offset)
                return false;
            data_offset += palette_size;
        }
        const std::uint32_t width = (std::uint32_t)(record.right - record.left + 1);
        const std::uint32_t height = (std::uint32_t)(record.bottom - record.top + 1);
        if((record.flags & 1) != 0)
        {
            const std::size_t source_stride = (width + 3) & ~(std::size_t)3;
            if(source_stride > (data_end - data_offset) / height)
                return false;
            record.pixels.resize((std::size_t)width * height);
            record.coverage.assign(record.pixels.size(), 1);
            for(std::uint32_t row = 0; row < height; ++row)
                std::copy_n(bytes.data() + data_offset + (std::size_t)(height - row - 1) * source_stride, width, record.pixels.begin() + (std::size_t)row * width);
        }
        else if((record.flags & 2) != 0)
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
        std::vector<std::uint8_t> bytes;
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
