#pragma once

#include "runtime_types.h"

namespace gag
{

GraphicsHostInitializationResult *initialize_runtime_graphics(const LegacyDisplayPixelFormat *format);

void synchronize_display_region(DisplayRectangle *rectangle, uint32_t mode);

uint32_t begin_display_target(void **pixels, DisplayRectangle *rectangle, uint32_t *pitch);

uint32_t initialize_direct_draw_runtime();

HRESULT WINAPI collect_direct_draw_display_mode(LegacyDirectDrawSurfaceDescriptor *descriptor, void *context);

uint32_t enumerate_direct_draw_display_modes();

uint32_t initialize_display_mode_host(HWND window, uint32_t options);

uint32_t enumerate_windows_display_modes();

DisplayMode *begin_display_mode_enumeration(uint32_t mask);

DisplayMode *get_next_display_mode(uint32_t mask);

DisplayMode *find_current_display_mode();

DisplayMode *get_current_display_mode();

DisplayMode *begin_available_display_modes(uint32_t mask);

DisplayMode *get_next_available_display_mode(uint32_t mask);

void shutdown_display_mode_host();

HWND find_top_level_display_window(HWND window);

uint32_t set_display_cooperative_mode(uint32_t mode);

void operate_display_surface(int32_t x, int32_t y, int32_t width, int32_t height, int32_t mode);

void *create_display_surface(int32_t width, int32_t height, const LegacyDisplayPixelFormat *format, uint32_t options);

void teardown_display_palette_surface();

UINT apply_display_palette(const PALETTEENTRY *palette, uint32_t update_flags);

void enable_display_palette_mode();

void disable_display_palette_mode();

HDC get_display_palette_dc();

HDC get_display_palette_dib_dc();

HBITMAP get_display_palette_bitmap();

HPALETTE get_display_palette_handle();

PALETTEENTRY *get_display_palette_entries();

void invalidate_game_framebuffer_rect(int32_t x, int32_t y, int32_t width, int32_t height);

} // namespace gag
