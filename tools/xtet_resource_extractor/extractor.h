#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>



namespace freegag
{

std::vector<uint8_t> extract_xtet_sfs(const std::filesystem::path &source);

void extract_xtet_resource(const std::filesystem::path &destination, const std::filesystem::path &source);

}
