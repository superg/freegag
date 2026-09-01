#include "resource_provider.h"
#include <filesystem>
#include <fstream>
#include <limits>
#include <system_error>
#include "../portable_path.h"

namespace xtet
{

bool load_sfs_file(const char *sfs_name, std::vector<uint8_t> &bytes, std::string &error)
{
    bytes.clear();
    error.clear();
    if(sfs_name == nullptr || *sfs_name == '\0')
    {
        error = "No XTET SFS filename was supplied.";
        return false;
    }
    const std::string display_name(sfs_name);

    std::filesystem::path payload_path;
    if(!freegag::resolve_existing_host_path_case_insensitive(sfs_name, &payload_path))
    {
        error = "Unable to open " + display_name + ".";
        return false;
    }
    std::error_code size_error;
    const uintmax_t payload_size = std::filesystem::file_size(payload_path, size_error);
    if(size_error)
    {
        error = "Unable to open " + display_name + ".";
        return false;
    }
    if(payload_size == 0)
    {
        error = display_name + " is empty.";
        return false;
    }
    if(payload_size > bytes.max_size() || payload_size > static_cast<uintmax_t>((std::numeric_limits<std::streamsize>::max)()))
    {
        error = display_name + " is too large to load.";
        return false;
    }

    bytes.resize(static_cast<size_t>(payload_size));
    std::ifstream stream(payload_path, std::ios::binary);
    if(!stream || !stream.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size())))
    {
        bytes.clear();
        error = "Unable to read " + display_name + ".";
        return false;
    }
    return true;
}

} // namespace xtet
