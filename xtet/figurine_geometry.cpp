#include "figurine_geometry.h"
#include <algorithm>

namespace xtet
{
namespace
{

constexpr std::array<FigurineTemplate, 10> templates{
    FigurineTemplate{ 0, 0, 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0 },
    FigurineTemplate{ 0, 0, 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0 },
    FigurineTemplate{ 0, 0, 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
    FigurineTemplate{ 0, 0, 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 },
    FigurineTemplate{ 0, 0, 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    FigurineTemplate{ 0, 0, 2, 0, 0, 0, 0, 2, 1, 1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    FigurineTemplate{ 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    FigurineTemplate{ 0, 0, 2, 1, 1, 0, 0, 2, 0, 0, 0, 0, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    FigurineTemplate{ 0, 0, 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    FigurineTemplate{ 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
};

FigurineTemplate orient_template(const FigurineTemplate &source, int8_t orientation)
{
    FigurineTemplate result{};
    for(size_t row = 0; row < 5; ++row)
        for(size_t column = 0; column < 5; ++column)
        {
            size_t destination_row = row;
            size_t destination_column = column;
            switch(orientation)
            {
            case 2:
                destination_row = column;
                destination_column = 4 - row;
                break;
            case 3:
                destination_row = 4 - row;
                destination_column = 4 - column;
                break;
            case 4:
                destination_row = 4 - column;
                destination_column = row;
                break;
            case -4:
                destination_row = column;
                destination_column = row;
                break;
            case -3:
                destination_row = 4 - row;
                destination_column = column;
                break;
            case -2:
                destination_row = 4 - column;
                destination_column = 4 - row;
                break;
            case -1:
                destination_row = row;
                destination_column = 4 - column;
                break;
            default:
                break;
            }
            result[destination_row * 5 + destination_column] = source[row * 5 + column];
        }
    return result;
}

std::array<int, 4> occupied_bounds(const FigurineTemplate &shape)
{
    std::array<int, 4> bounds{ 5, 5, -1, -1 };
    for(int row = 0; row < 5; ++row)
        for(int column = 0; column < 5; ++column)
            if(shape[(size_t)row * 5 + column] != 0)
            {
                bounds[0] = std::min(bounds[0], column);
                bounds[1] = std::min(bounds[1], row);
                bounds[2] = std::max(bounds[2], column);
                bounds[3] = std::max(bounds[3], row);
            }
    return bounds;
}

std::array<FigurineOffset, 8> build_offsets(const FigurineTemplate &shape)
{
    constexpr std::array<int8_t, 8> orientations{ 4, 1, 2, 3, -4, -1, -2, -3 };
    std::array<FigurineOffset, 8> offsets{};
    for(size_t index = 0; index < orientations.size(); ++index)
    {
        const int8_t current = orientations[index];
        const int8_t next = orientations[(index & 4) | ((index + 1) & 3)];
        const std::array<int, 4> current_bounds = occupied_bounds(orient_template(shape, current));
        const std::array<int, 4> next_bounds = occupied_bounds(orient_template(shape, next));
        offsets[index].x = (int8_t)(((current_bounds[2] - next_bounds[2]) - next_bounds[0] + current_bounds[0]) / 2);
        offsets[index].y = (int8_t)(((current_bounds[3] - next_bounds[3]) - next_bounds[1] + current_bounds[1]) / 2);
    }
    return offsets;
}

} // namespace

FigurineGeometryTables build_figurine_geometry_tables()
{
    FigurineGeometryTables result;
    for(size_t index = 0; index < result.first_family.size(); ++index)
        result.first_family[index] = build_offsets(templates[index]);
    for(size_t index = 0; index < result.second_family.size(); ++index)
        result.second_family[index] = build_offsets(templates[index]);
    return result;
}

bool get_oriented_figurine_template(bool first_family, uint8_t shape_index, int8_t orientation, FigurineTemplate &result)
{
    const size_t shape_count = first_family ? 5 : 10;
    if(shape_index >= shape_count || orientation < -4 || orientation > 4 || orientation == 0)
        return false;
    result = orient_template(templates[shape_index], orientation);
    return true;
}

} // namespace xtet
