#pragma once

#include <windows.h>
#include <array>
#include <cstddef>
#include <cstdint>

#if defined(_M_IX86) || defined(__i386__)
#define XTET_ABI __fastcall
#else
#define XTET_ABI
#endif

namespace xtet
{

struct PcmFormat;

constexpr UINT kGameMessage = 0x7ffc;
constexpr std::size_t kCallbackCount = 35;

#pragma pack(push, 1)
struct GameHostContext
{
    HWND window;
    std::uint32_t unknown04{};
    std::uint32_t bits_per_pixel{ 8 };
    std::array<std::byte, 0x14> unknown0c{};
    std::uint16_t width{ 640 };
    std::uint16_t height{ 480 };
    std::uint32_t unknown24{};
    std::uint32_t unknown28{};
    void *framebuffer{};
    std::array<std::byte, 0x10> unknown30{};
};

struct GameResultDescriptor
{
    std::uint32_t type;
    std::uint32_t reserved;
    std::uint32_t size;
    const void *data;
};
#pragma pack(pop)

#if defined(_M_IX86) || defined(__i386__)
static_assert(sizeof(GameHostContext) == 0x40);
static_assert(offsetof(GameHostContext, width) == 0x20);
static_assert(offsetof(GameHostContext, framebuffer) == 0x2c);
static_assert(sizeof(GameResultDescriptor) == 0x10);
#endif

using GameInit = void(XTET_ABI *)(GameHostContext *, void **);
using GameWndProc = std::uint32_t(XTET_ABI *)(HWND, UINT, WPARAM, LPARAM);
using GameExec = void(XTET_ABI *)(std::uint32_t);
using DirtyRegionCallback = void(XTET_ABI *)(std::int32_t, std::int32_t, std::int32_t, std::int32_t);
using SoundCreateCallback = std::uint32_t(XTET_ABI *)(const PcmFormat *);
using SoundDestroyCallback = void(XTET_ABI *)(std::uint32_t);
using SoundQueueCallback = std::uint32_t(XTET_ABI *)(std::uint32_t, const void *, std::uint32_t, std::int32_t);
using SoundControlCallback = std::uint32_t(XTET_ABI *)(std::uint32_t, std::int32_t);

} // namespace xtet
