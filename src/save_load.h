#pragma once

#include <stdint.h>

namespace gag
{

struct ApplicationState;
struct DisplaySceneNode;
struct RuntimeTreeNode;

struct ArchiveCommentEntry
{
    char path[0x104];
    char comment[0x104];
    uint64_t modification_time;
    int32_t numeric_identifier;
    char file_name[0x104];
};

struct ArchiveCommentCollection
{
    ArchiveCommentEntry *entries;
    uint32_t count;
    uint32_t capacity;
    uint32_t next_identifier;
};

struct VirtualScriptResource
{
    const char *data;
    uint32_t size;
    uint32_t resource_type;
};

enum class SaveLoadScreenMode
{
    load,
    save
};

struct ScriptedSaveLoadPersistenceApi
{
    void *(*capture_state)(void *game_context, uint32_t *size, int mode);
    uintptr_t (*get_script_state)();
    void (*free_memory)(void *memory);
    bool (*write_state)(char *path, char *name, void *bitmap, uintptr_t script_state);
    uint32_t (*get_file_attributes)(const char *path);
};

uint32_t enumerate_archive_comment_entries(const char *directory, const char *extension, ArchiveCommentCollection *collection);
void destroy_archive_comment_collection(ArchiveCommentCollection *collection);
bool find_save_load_virtual_script(const char *name, VirtualScriptResource *resource);
const char *save_load_screen_section(SaveLoadScreenMode mode);
bool request_scripted_save_load_screen(SaveLoadScreenMode mode, ApplicationState *state);
bool handle_scripted_save_load_message(uintptr_t message, ApplicationState *state);
void on_scripted_save_load_tree_rebuilt(RuntimeTreeNode *tree);
void on_scripted_save_load_tree_resources_destroyed(RuntimeTreeNode *tree);

#if defined(GAG_TESTING)
uint32_t get_scripted_save_load_entry_count_for_testing();
uint32_t get_scripted_save_load_selection_for_testing();
const ArchiveCommentEntry *get_scripted_save_load_selected_entry_for_testing();
void set_scripted_save_load_collection_for_testing(ArchiveCommentCollection collection, uint32_t selection);
const char *get_scripted_save_load_current_name_for_testing();
bool get_scripted_save_load_editing_for_testing();
void set_scripted_save_load_persistence_api_for_testing(const ScriptedSaveLoadPersistenceApi &api);
void finish_scripted_save_name_for_testing(const char *input, ApplicationState *state);
void prepare_scripted_save_name_display_for_testing(const char *name, char *display);
bool decode_scripted_save_preview_for_testing(const uint8_t *data, uint32_t size, DisplaySceneNode *scene);
bool get_scripted_save_preview_palette_for_testing(const uint8_t *data, uint32_t size, uint32_t *palette);
#endif

} // namespace gag
