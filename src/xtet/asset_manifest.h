#pragma once

#include <string>
#include <vector>
#include "sfs_archive.h"

namespace xtet
{

struct AssetManifest
{
    std::vector<std::string> script_paths;
    std::vector<std::string> bitmap_paths;
    std::vector<std::string> wave_paths;
};

bool load_asset_manifest(const SfsArchive &archive, const std::vector<std::string> &root_scripts, AssetManifest &manifest);

} // namespace xtet
