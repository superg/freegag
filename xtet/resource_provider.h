#pragma once

#include <windows.h>
#include <cstddef>
#include <cstdint>

namespace xtet
{

struct ResourceView
{
    const std::uint8_t *data{};
    std::size_t size{};
};

ResourceView load_embedded_sfs(HMODULE module);

} // namespace xtet
