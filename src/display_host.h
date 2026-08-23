#pragma once

#include "runtime_types.h"

namespace gag
{

constexpr UINT sdl_presenter_message = 0x7ffb;

GraphicsHostInitializationResult *initialize_runtime_graphics();

uint32_t initialize_sdl_presenter(HWND window, uint32_t options);

void shutdown_sdl_presenter();

void begin_sdl_presenter_shutdown();

void synchronize_display_region(DisplayRectangle *rectangle, uint32_t mode);

void operate_display_surface(int32_t x, int32_t y, int32_t width, int32_t height, int32_t mode);

uint32_t begin_display_target(void **pixels, DisplayRectangle *rectangle, uint32_t *pitch);

uint32_t end_display_target();

void *create_display_surface(int32_t width, int32_t height);

void teardown_display_palette_surface();

UINT apply_display_palette(const PALETTEENTRY *palette, uint32_t update_flags);

void enable_display_palette_mode();

void disable_display_palette_mode();

PALETTEENTRY *get_display_palette_entries();

void invalidate_game_framebuffer_rect(int32_t x, int32_t y, int32_t width, int32_t height);

void handle_sdl_presenter_message();

void repaint_sdl_presenter();

} // namespace gag
