#pragma once

#include <memory>
#include "runtime_types.h"

namespace gag
{
class SharedBinaryInputState;

RuntimeResourceConstructionPlan prepare_runtime_resource_construction(uint32_t scene_identifier, int32_t x, int32_t y, uint32_t flags);

void *construct_runtime_resource(char *path, uint32_t scene_identifier, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t scale_or_loop, uint32_t flags);

uint32_t update_runtime_resource_visibility(DisplayTraversalState *state);

void request_runtime_resource_destruction(void *identity);

void queue_runtime_resource_destruction(void *identity, bool decrement_wait_count);

void drain_runtime_resource_destructions();

uint32_t query_runtime_resource_frame_limit(void *identity);

uint32_t query_runtime_resource_playback_flags(void *identity);

void set_runtime_property_value(uint32_t value);

uint32_t get_runtime_property_value();

uint16_t query_runtime_resource_frame_number(void *identity);

void select_runtime_resource(char *path);

uint32_t destroy_runtime_resource(void *identity);

bool release_runtime_memory_resource(const char *name);

bool release_runtime_memory_resource_by_data(void *data);

uint32_t release_runtime_streamed_resource(AsyncFileRecord *record);

void destroy_runtime_tree_resources(void *identity);

void update_runtime_resource_scene_region(intptr_t scene_identifier, int32_t x, int32_t y, int32_t width, int32_t height);

void copy_runtime_bitmap_region(RuntimeMediaBackend *backend, DisplayRectangle *rectangle);

uint32_t render_runtime_bitmap_backend_region(void *identity, DisplayRectangle *rectangle);

void select_runtime_scene_transition(uint32_t flags);

void set_runtime_resource_state(void *identity, uint32_t state);

void apply_immediate_runtime_scene_transition(uint32_t unused, uint32_t flags);

void apply_palette_runtime_scene_transition(uint32_t step, uint32_t flags);

void apply_rectangle_runtime_scene_transition(uint8_t size, uint32_t flags);

void finalize_runtime_resource_destruction(void *identity);

void release_runtime_lock_record(RuntimeLockRecord *record);

RuntimeLockRecord *acquire_runtime_lock_record(void *child_identity);

void set_runtime_resource_loop_count(void *identity, uint32_t count);

void switch_runtime_scene(void *identity);

void reset_runtime_display_state();

uint32_t shutdown_runtime_display();

uint32_t query_runtime_scene_flags(void *identity);

void wait_for_runtime_resource_count(uint32_t count);

void update_runtime_scene_position(void *identity, int32_t x, int32_t y);

void build_runtime_resource_path(char *destination, const char *source);

void update_runtime_resource_host(const char *path, int32_t reset);

uint32_t detect_runtime_resource_type(const char *path);

void *open_runtime_cdf_entry_stream(CdfArchive *archive, const char *name);

void load_runtime_resource(const char *path, void **data, uint32_t *size, int32_t *storage, uint32_t flags);

uint32_t extract_runtime_drive_prefix(char *destination, const char *source);

AsyncFileHost *acquire_async_file_host(AsyncFileHost *identity);

AsyncFileHost *create_async_file_host(const char *root, uint32_t requested_bytes, int32_t mode);

void advance_async_host_write(AsyncFileHost *host, uint32_t bytes);

void advance_async_host_read(AsyncFileHost *host, uint32_t bytes);

void invalidate_shared_async_records(AsyncFileRecord *record);

void position_async_host(AsyncFileHost *host, uint32_t offset);

void seek_async_host(AsyncFileHost *host, uint32_t offset);

uint32_t copy_async_host_bytes(AsyncFileHost *host, void *destination, uint32_t bytes, uint32_t *total_bytes);

void activate_async_file_record(AsyncFileRecord *record);

void handle_async_host_short_read(AsyncFileHost *host);

void run_async_file_worker(AsyncFileHost *host);

void release_async_file_host(AsyncFileHost *identity);

uint32_t destroy_async_file_host(AsyncFileHost *identity);

uint32_t shutdown_async_file_subsystem();

AsyncFileRecord *acquire_async_file_record(AsyncFileRecord *identity);

void release_async_file_record(AsyncFileRecord *identity);

void set_async_file_host_mode(AsyncFileHost *identity, int32_t mode);

uint32_t get_async_file_size(AsyncFileRecord *identity);

uint32_t get_async_file_position(AsyncFileRecord *identity);

uint32_t set_async_file_position(AsyncFileRecord *identity, uint32_t position);

AsyncFileRecord *open_async_file_record(AsyncFileHost *host_identity, const char *path, uint32_t start_offset, uint32_t end_offset, uint32_t flags);
AsyncFileRecord *open_async_file_record(AsyncFileHost *host_identity, std::shared_ptr<SharedBinaryInputState> file, uint32_t start_offset, uint32_t end_offset, uint32_t flags);

AsyncFileRecord *duplicate_async_file_record(AsyncFileRecord *identity, uint32_t start_offset, uint32_t end_offset, uint32_t flags);

uint32_t close_async_file_record(AsyncFileRecord *identity);

uint32_t read_async_file_record(AsyncFileRecord *identity, void *destination, uint32_t bytes, uint32_t *bytes_read, int32_t force_host_buffer);

void reset_runtime_session();

} // namespace gag
