#pragma once

#include <array>
#include <stdint.h>

namespace xtet
{

using FigurineTemplate = std::array<uint8_t, 25>;

struct FigurineOffset
{
    int8_t x{};
    int8_t y{};

    bool operator==(const FigurineOffset &other) const
    {
        return x == other.x && y == other.y;
    }
};

struct FigurineGeometryTables
{
    std::array<std::array<FigurineOffset, 8>, 5> first_family{};
    std::array<std::array<FigurineOffset, 8>, 10> second_family{};
};

FigurineGeometryTables build_figurine_geometry_tables();
bool get_oriented_figurine_template(bool first_family, uint8_t shape_index, int8_t orientation, FigurineTemplate &result);

} // namespace xtet
