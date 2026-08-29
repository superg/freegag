#pragma once

#include <stdint.h>

namespace freegag
{

struct ApplicationState;
struct DisplaySceneNode;
struct RuntimeTreeNode;

struct ArchiveCommentEntry
{
    char path[0x104];
    char comment[0x104];
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

enum class SaveLoadScreenMode
{
    LOAD,
    SAVE
};

enum ArchiveCommentEnumerationResult : uint32_t
{
    ARCHIVE_COMMENT_ENUMERATION_SUCCESS = 0,
    ARCHIVE_COMMENT_ENUMERATION_EMPTY = 2,
    ARCHIVE_COMMENT_ENUMERATION_FAILED = 0x00010000
};

uint32_t enumerate_archive_comment_entries(const char *directory, const char *extension, ArchiveCommentCollection *collection);
void destroy_archive_comment_collection(ArchiveCommentCollection *collection);
const char *save_load_screen_section(SaveLoadScreenMode mode);
bool request_scripted_save_load_screen(SaveLoadScreenMode mode, ApplicationState *state);
bool handle_scripted_save_load_message(uintptr_t message, ApplicationState *state);
void on_scripted_save_load_tree_rebuilt(RuntimeTreeNode *tree);
void on_scripted_save_load_tree_resources_destroyed(RuntimeTreeNode *tree);

} // namespace freegag
