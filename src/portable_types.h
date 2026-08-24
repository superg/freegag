#pragma once

#include <cstdint>
#include <cstring>

namespace gag
{
struct PaletteEntry
{
    uint8_t peRed{};
    uint8_t peGreen{};
    uint8_t peBlue{};
    uint8_t peFlags{};
};

struct BitmapColor
{
    uint8_t rgbBlue{};
    uint8_t rgbGreen{};
    uint8_t rgbRed{};
    uint8_t rgbReserved{};
};

#pragma pack(push, 1)
struct BitmapFileHeader
{
    uint16_t bfType{};
    uint32_t bfSize{};
    uint16_t bfReserved1{};
    uint16_t bfReserved2{};
    uint32_t bfOffBits{};
};
#pragma pack(pop)

struct BitmapInfoHeader
{
    uint32_t biSize{};
    int32_t biWidth{};
    int32_t biHeight{};
    uint16_t biPlanes{};
    uint16_t biBitCount{};
    uint32_t biCompression{};
    uint32_t biSizeImage{};
    int32_t biXPelsPerMeter{};
    int32_t biYPelsPerMeter{};
    uint32_t biClrUsed{};
    uint32_t biClrImportant{};
};

struct PortableRectangle
{
    int32_t left{};
    int32_t top{};
    int32_t right{};
    int32_t bottom{};
};

static_assert(sizeof(BitmapFileHeader) == 14);
static_assert(sizeof(BitmapInfoHeader) == 40);
static_assert(sizeof(BitmapColor) == 4);

inline uint16_t read_little_endian_u16(const uint8_t *bytes)
{
    return static_cast<uint16_t>(bytes[0] | static_cast<uint16_t>(bytes[1]) << 8);
}

inline uint32_t read_little_endian_u32(const uint8_t *bytes)
{
    return static_cast<uint32_t>(bytes[0]) | static_cast<uint32_t>(bytes[1]) << 8 | static_cast<uint32_t>(bytes[2]) << 16 | static_cast<uint32_t>(bytes[3]) << 24;
}

inline void write_little_endian_u16(uint8_t *bytes, uint16_t value)
{
    bytes[0] = static_cast<uint8_t>(value);
    bytes[1] = static_cast<uint8_t>(value >> 8);
}

inline void write_little_endian_u32(uint8_t *bytes, uint32_t value)
{
    bytes[0] = static_cast<uint8_t>(value);
    bytes[1] = static_cast<uint8_t>(value >> 8);
    bytes[2] = static_cast<uint8_t>(value >> 16);
    bytes[3] = static_cast<uint8_t>(value >> 24);
}

inline BitmapFileHeader decode_bitmap_file_header(const uint8_t *bytes)
{
    return { read_little_endian_u16(bytes), read_little_endian_u32(bytes + 2), read_little_endian_u16(bytes + 6), read_little_endian_u16(bytes + 8), read_little_endian_u32(bytes + 10) };
}

inline BitmapInfoHeader decode_bitmap_info_header(const uint8_t *bytes)
{
    return { read_little_endian_u32(bytes), static_cast<int32_t>(read_little_endian_u32(bytes + 4)), static_cast<int32_t>(read_little_endian_u32(bytes + 8)), read_little_endian_u16(bytes + 12),
        read_little_endian_u16(bytes + 14), read_little_endian_u32(bytes + 16), read_little_endian_u32(bytes + 20), static_cast<int32_t>(read_little_endian_u32(bytes + 24)),
        static_cast<int32_t>(read_little_endian_u32(bytes + 28)), read_little_endian_u32(bytes + 32), read_little_endian_u32(bytes + 36) };
}

inline void encode_bitmap_file_header(uint8_t *bytes, const BitmapFileHeader &header)
{
    write_little_endian_u16(bytes, header.bfType);
    write_little_endian_u32(bytes + 2, header.bfSize);
    write_little_endian_u16(bytes + 6, header.bfReserved1);
    write_little_endian_u16(bytes + 8, header.bfReserved2);
    write_little_endian_u32(bytes + 10, header.bfOffBits);
}

inline void encode_bitmap_info_header(uint8_t *bytes, const BitmapInfoHeader &header)
{
    write_little_endian_u32(bytes, header.biSize);
    write_little_endian_u32(bytes + 4, static_cast<uint32_t>(header.biWidth));
    write_little_endian_u32(bytes + 8, static_cast<uint32_t>(header.biHeight));
    write_little_endian_u16(bytes + 12, header.biPlanes);
    write_little_endian_u16(bytes + 14, header.biBitCount);
    write_little_endian_u32(bytes + 16, header.biCompression);
    write_little_endian_u32(bytes + 20, header.biSizeImage);
    write_little_endian_u32(bytes + 24, static_cast<uint32_t>(header.biXPelsPerMeter));
    write_little_endian_u32(bytes + 28, static_cast<uint32_t>(header.biYPelsPerMeter));
    write_little_endian_u32(bytes + 32, header.biClrUsed);
    write_little_endian_u32(bytes + 36, header.biClrImportant);
}
} // namespace gag
