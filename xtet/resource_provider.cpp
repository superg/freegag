#include "resource_provider.h"
#include <limits>

namespace xtet
{

ResourceView load_embedded_sfs(HMODULE module)
{
    const HRSRC resource = FindResourceA(module, "XTETSFS", RT_RCDATA);
    if(!resource)
        return {};
    const DWORD size = SizeofResource(module, resource);
    const HGLOBAL loaded = LoadResource(module, resource);
    const void *data = loaded ? LockResource(loaded) : nullptr;
    if(!data || size == 0 || (std::uintmax_t)size > (std::uintmax_t)std::numeric_limits<std::size_t>::max())
        return {};
    return { (const std::uint8_t *)data, (std::size_t)size };
}

} // namespace xtet
