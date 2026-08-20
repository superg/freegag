#include "resource_provider.h"
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <limits>
#include <system_error>

namespace xtet
{

bool load_executable_sfs(const char *sfs_name, std::vector<uint8_t> &bytes, std::string &error)
{
    bytes.clear();
    error.clear();
    if(sfs_name == nullptr || *sfs_name == '\0')
    {
        error = "No XTET SFS filename was supplied.";
        return false;
    }
    const std::string display_name(sfs_name);

    std::wstring executable_path(MAX_PATH, L'\0');
    for(;;)
    {
        const DWORD length = GetModuleFileNameW(nullptr, executable_path.data(), static_cast<DWORD>(executable_path.size()));
        if(length == 0)
        {
            error = "Unable to determine the gag.exe directory for " + display_name + ".";
            return false;
        }
        if(length < executable_path.size() - 1)
        {
            executable_path.resize(length);
            break;
        }
        if(executable_path.size() >= 32768)
        {
            error = "The gag.exe path is too long to locate " + display_name + ".";
            return false;
        }
        executable_path.resize(executable_path.size() * 2);
    }

    const std::filesystem::path payload_path = std::filesystem::path(executable_path).parent_path() / sfs_name;
    std::error_code size_error;
    const uintmax_t payload_size = std::filesystem::file_size(payload_path, size_error);
    if(size_error)
    {
        error = "Unable to open " + display_name + " beside gag.exe.";
        return false;
    }
    if(payload_size == 0)
    {
        error = display_name + " beside gag.exe is empty.";
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
        error = "Unable to read " + display_name + " beside gag.exe.";
        return false;
    }
    return true;
}

} // namespace xtet
