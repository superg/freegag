#pragma once

#include <array>
#include <cstdint>

namespace xtet
{

using FigurineTemplate = std::array<std::uint8_t, 25>;

struct FigurineOffset
{
    std::int8_t x{};
    std::int8_t y{};

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
bool get_oriented_figurine_template(bool first_family, std::uint8_t shape_index, std::int8_t orientation, FigurineTemplate &result);

} // namespace xtet
