#include "synthesized_resource.h"
#include <windows.h>
#include <array>
#include <cstring>
#include <limits>
#include <zlib.h>
#include "cdf_archive.h"
#include "save_load_bg_patch.h"

namespace gag
{
namespace
{
constexpr int32_t synthesized_width = 640;
constexpr int32_t synthesized_height = 480;
constexpr uint32_t palette_count = 256;
constexpr char base85_alphabet[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.-:+=^!/*?&<>()[]{}@%$#";

struct IndexedBitmapView
{
    const RGBQUAD *palette;
    const uint8_t *pixels;
    uint32_t row_stride;
};

struct BlitRegion
{
    uint32_t left;
    uint32_t top;
    uint32_t right;
    uint32_t bottom;
};

using SynthesizedResourceBuilder = bool (*)(CdfArchive *archive, const SynthesizedResourceSourceApi &source_api, SynthesizedResource *resource);

struct SynthesizedResourceDefinition
{
    const char *name;
    uint32_t resource_type;
    SynthesizedResourceBuilder builder;
};

const char *file_name_from_path(const char *path)
{
    const char *name = path;
    for(const char *cursor = path; *cursor != '\0'; ++cursor)
    {
        if(*cursor == '\\' || *cursor == '/')
        {
            name = cursor + 1;
        }
    }
    return name;
}

bool load_source(CdfArchive *archive, const char *name, const SynthesizedResourceSourceApi &source_api, std::vector<uint8_t> *data)
{
    const uint32_t size = source_api.get_size(archive, 0, name);
    if(size == 0)
    {
        return false;
    }
    data->resize(size);
    if(source_api.read(archive, 0, name, data->data()) == 0)
    {
        data->clear();
        return false;
    }
    return true;
}

bool validate_source(const std::vector<uint8_t> &data, IndexedBitmapView *view)
{
    if(data.size() < sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER))
    {
        return false;
    }
    BITMAPFILEHEADER file_header;
    BITMAPINFOHEADER info_header;
    std::memcpy(&file_header, data.data(), sizeof(file_header));
    std::memcpy(&info_header, data.data() + sizeof(file_header), sizeof(info_header));
    constexpr uint32_t palette_offset = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    constexpr uint32_t palette_size = palette_count * sizeof(RGBQUAD);
    constexpr uint32_t row_stride = (synthesized_width + 3u) & ~3u;
    constexpr uint32_t pixel_size = row_stride * synthesized_height;
    if(file_header.bfType != 0x4d42 || info_header.biSize != sizeof(BITMAPINFOHEADER) || info_header.biWidth != synthesized_width || info_header.biHeight != synthesized_height
        || info_header.biPlanes != 1 || info_header.biBitCount != 8 || info_header.biCompression != BI_RGB || file_header.bfOffBits < palette_offset + palette_size
        || file_header.bfOffBits > data.size() || pixel_size > data.size() - file_header.bfOffBits
        || (file_header.bfSize != 0 && (file_header.bfSize > data.size() || file_header.bfSize < file_header.bfOffBits + pixel_size)))
    {
        return false;
    }
    view->palette = reinterpret_cast<const RGBQUAD *>(data.data() + palette_offset);
    view->pixels = data.data() + file_header.bfOffBits;
    view->row_stride = row_stride;
    return true;
}

uint8_t closest_palette_index(const RGBQUAD &color, const RGBQUAD *palette)
{
    uint32_t best_distance = std::numeric_limits<uint32_t>::max();
    uint8_t best_index = 0;
    for(uint32_t index = 0; index < palette_count; ++index)
    {
        const int32_t red = static_cast<int32_t>(color.rgbRed) - palette[index].rgbRed;
        const int32_t green = static_cast<int32_t>(color.rgbGreen) - palette[index].rgbGreen;
        const int32_t blue = static_cast<int32_t>(color.rgbBlue) - palette[index].rgbBlue;
        const uint32_t distance = static_cast<uint32_t>(red * red + green * green + blue * blue);
        if(distance < best_distance)
        {
            best_distance = distance;
            best_index = static_cast<uint8_t>(index);
        }
    }
    return best_index;
}

std::array<uint8_t, palette_count> build_palette_mapping(const IndexedBitmapView &source, const RGBQUAD *destination_palette)
{
    std::array<uint8_t, palette_count> mapping{};
    for(uint32_t index = 0; index < palette_count; ++index)
    {
        mapping[index] = closest_palette_index(source.palette[index], destination_palette);
    }
    return mapping;
}

uint8_t source_pixel(const IndexedBitmapView &source, uint32_t x, uint32_t y)
{
    return source.pixels[(synthesized_height - 1u - y) * source.row_stride + x];
}

uint8_t *output_pixel(uint8_t *pixels, uint32_t stride, uint32_t x, uint32_t y)
{
    return pixels + (synthesized_height - 1u - y) * stride + x;
}

void blit(const IndexedBitmapView &source, const std::array<uint8_t, palette_count> &mapping, uint8_t *destination_pixels, uint32_t destination_stride, const BlitRegion &region)
{
    for(uint32_t y = region.top; y < region.bottom; ++y)
    {
        for(uint32_t x = region.left; x < region.right; ++x)
        {
            *output_pixel(destination_pixels, destination_stride, x, y) = mapping[source_pixel(source, x, y)];
        }
    }
}

bool decode_manual_patch(std::vector<uint8_t> *pixels)
{
    constexpr size_t encoded_size = sizeof(save_load_background_manual_patch::encoded_pixels) - 1;
    static_assert(encoded_size % 5 == 0);
    std::vector<uint8_t> compressed(encoded_size / 5 * 4);
    size_t output = 0;
    for(size_t offset = 0; offset < encoded_size; offset += 5)
    {
        uint64_t value = 0;
        for(size_t digit = 0; digit < 5; ++digit)
        {
            const char *position = std::strchr(base85_alphabet, save_load_background_manual_patch::encoded_pixels[offset + digit]);
            if(position == nullptr)
            {
                return false;
            }
            value = value * 85 + static_cast<uint64_t>(position - base85_alphabet);
        }
        if(value > std::numeric_limits<uint32_t>::max())
        {
            return false;
        }
        compressed[output++] = static_cast<uint8_t>(value >> 24);
        compressed[output++] = static_cast<uint8_t>(value >> 16);
        compressed[output++] = static_cast<uint8_t>(value >> 8);
        compressed[output++] = static_cast<uint8_t>(value);
    }
    compressed.resize(save_load_background_manual_patch::compressed_size);
    pixels->resize(save_load_background_manual_patch::decoded_size);
    uLongf decoded_size = static_cast<uLongf>(pixels->size());
    return uncompress(pixels->data(), &decoded_size, compressed.data(), static_cast<uLong>(compressed.size())) == Z_OK && decoded_size == pixels->size();
}

bool apply_manual_patch(uint8_t *destination_pixels, uint32_t destination_stride)
{
    std::vector<uint8_t> pixels;
    if(!decode_manual_patch(&pixels))
    {
        return false;
    }
    size_t offset = 0;
    for(const save_load_background_manual_patch::Region &region : save_load_background_manual_patch::regions)
    {
        for(uint32_t y = region.top; y < region.bottom; ++y)
        {
            for(uint32_t x = region.left; x < region.right; ++x)
            {
                *output_pixel(destination_pixels, destination_stride, x, y) = pixels[offset++];
            }
        }
    }
    return offset == pixels.size();
}

bool synthesize_save_load_background(CdfArchive *archive, const SynthesizedResourceSourceApi &source_api, SynthesizedResource *resource)
{
    std::vector<uint8_t> fullscreen;
    std::vector<uint8_t> help;
    std::vector<uint8_t> help_page;
    if(!load_source(archive, "FSCR0000.BMP", source_api, &fullscreen) || !load_source(archive, "HELP0000.BMP", source_api, &help) || !load_source(archive, "HELP0001.BMP", source_api, &help_page))
    {
        return false;
    }
    IndexedBitmapView fullscreen_view{};
    IndexedBitmapView help_view{};
    IndexedBitmapView help_page_view{};
    if(!validate_source(fullscreen, &fullscreen_view) || !validate_source(help, &help_view) || !validate_source(help_page, &help_page_view))
    {
        return false;
    }
    const std::array<uint8_t, palette_count> fullscreen_mapping = build_palette_mapping(fullscreen_view, help_view.palette);
    const std::array<uint8_t, palette_count> help_page_mapping = build_palette_mapping(help_page_view, help_view.palette);
    resource->data = help;
    BITMAPFILEHEADER output_header;
    std::memcpy(&output_header, resource->data.data(), sizeof(output_header));
    uint8_t *output_pixels = resource->data.data() + output_header.bfOffBits;
    blit(fullscreen_view, fullscreen_mapping, output_pixels, help_view.row_stride, { 0, 0, 280, 300 });
    blit(help_page_view, help_page_mapping, output_pixels, help_view.row_stride, { 530, 420, 610, 450 });
    return apply_manual_patch(output_pixels, help_view.row_stride);
}

constexpr SynthesizedResourceDefinition synthesized_resources[]{
    { "FGSL0000.BMP", 1, synthesize_save_load_background }
};

const SynthesizedResourceDefinition *find_definition(const char *path)
{
    if(path == nullptr)
    {
        return nullptr;
    }
    const char *name = file_name_from_path(path);
    for(const SynthesizedResourceDefinition &definition : synthesized_resources)
    {
        if(_stricmp(name, definition.name) == 0)
        {
            return &definition;
        }
    }
    return nullptr;
}
}

uint32_t get_synthesized_resource_type(const char *name)
{
    const SynthesizedResourceDefinition *definition = find_definition(name);
    return definition == nullptr ? 0 : definition->resource_type;
}

bool synthesize_resource(CdfArchive *archive, const char *name, const SynthesizedResourceSourceApi &source_api, SynthesizedResource *resource)
{
    const SynthesizedResourceDefinition *definition = find_definition(name);
    if(archive == nullptr || definition == nullptr || source_api.get_size == nullptr || source_api.read == nullptr || resource == nullptr)
    {
        return false;
    }
    *resource = {};
    if(!definition->builder(archive, source_api, resource))
    {
        return false;
    }
    resource->resource_type = definition->resource_type;
    return true;
}

} // namespace gag
