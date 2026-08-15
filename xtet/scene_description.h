#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include "sfs_archive.h"

namespace xtet
{

enum class SceneNodeType
{
    sprites,
    sprite_bitmap,
    bitmap,
    wave,
    empty
};

struct ScenePoint
{
    std::int32_t x{};
    std::int32_t y{};
};

struct SceneNode
{
    SceneNodeType type{};
    std::string source_script;
    std::vector<std::string> links;
    std::optional<ScenePoint> size;
    std::optional<ScenePoint> map_size;
    std::optional<ScenePoint> position;
    std::optional<bool> shown;
    std::optional<bool> viewed;
    std::optional<bool> transparent;
    std::optional<std::uint8_t> fill_index;
    std::string loaded_path;
    std::optional<ScenePoint> created_size;
    std::vector<SceneNode> children;
};

struct SceneDescription
{
    std::vector<SceneNode> roots;
};

bool load_scene_description(const SfsArchive &archive, const std::vector<std::string> &root_scripts, SceneDescription &description);
std::vector<const SceneNode *> find_scene_links(const SceneDescription &description, const std::string &link);

} // namespace xtet
