#pragma once

#include "pcm_format.h"
#include "portable_types.h"
#include "runtime_input.h"
#include "runtime_services.h"
#include "script_types.h"

namespace freegag
{
inline constexpr uint32_t RUNTIME_RESOURCE_FRAME_LIMIT_UNBOUNDED = 0xffffffff;

enum RuntimeResourceFlag : uint32_t
{
    RUNTIME_RESOURCE_PRIMARY = 0x00000001,
    RUNTIME_RESOURCE_HALF_SIZE = 0x00000002,
    RUNTIME_RESOURCE_HIDDEN = 0x00000004,
    RUNTIME_RESOURCE_DEFERRED_LOAD = 0x00000010,
    RUNTIME_RESOURCE_INDEPENDENT_SCENE = 0x00000020,
    RUNTIME_RESOURCE_INDEXED_SOURCE = 0x00000040,
    RUNTIME_RESOURCE_INTERNAL_PRIMARY = 0x00000080,
    RUNTIME_RESOURCE_ONE_STEP = 0x00000200,
    RUNTIME_RESOURCE_LOOP = 0x00000400,
    RUNTIME_RESOURCE_NATURAL_MOUSE = 0x00010000,
    RUNTIME_RESOURCE_NO_SKIP = 0x00100000,
    RUNTIME_RESOURCE_NO_CLOSE = 0x01000000,
    RUNTIME_RESOURCE_HOST_MEMORY = 0x20000000
};

enum RuntimeResourceTypeFlag : uint32_t
{
    RUNTIME_RESOURCE_TYPE_LOW_FLAGS_MASK = 0x000000ff,
    RUNTIME_RESOURCE_TYPE_BITMAP = 0x00001000,
    RUNTIME_RESOURCE_TYPE_ANIMATION = 0x00002000,
    RUNTIME_RESOURCE_VISUAL_TYPE_MASK = RUNTIME_RESOURCE_TYPE_BITMAP | RUNTIME_RESOURCE_TYPE_ANIMATION,
    RUNTIME_RESOURCE_TYPE_SOUND = 0x00008000,
    RUNTIME_RESOURCE_TYPE_GENERIC = 0x00010000,
    RUNTIME_RESOURCE_GENERIC_CHILD_ATTACHED = 0x01000000,
    RUNTIME_RESOURCE_TYPE_MASK = 0x000ff000
};

inline constexpr uint32_t RUNTIME_RESOURCE_REFERENCE_COUNT_MASK = 0x0000ffff;

inline constexpr uint32_t RUNTIME_RESOURCE_PRESENTATION_DEFERRED = 0x00000001;

enum RuntimeResourceTransitionFlag : uint32_t
{
    RUNTIME_RESOURCE_TRANSITION_ACTIVATE = 0x00001000,
    RUNTIME_RESOURCE_TRANSITION_DEACTIVATE = 0x00002000,
    RUNTIME_RESOURCE_TRANSITION_SKIP_FADE = 0x20000000
};

enum RuntimeSceneTransitionFlag : uint32_t
{
    RUNTIME_SCENE_TRANSITION_IMMEDIATE = 0x00000001,
    RUNTIME_SCENE_TRANSITION_PALETTE = 0x00000002,
    RUNTIME_SCENE_TRANSITION_RECTANGLE = 0x00000004,
    RUNTIME_SCENE_TRANSITION_AVAILABILITY_MASK = (1u << 4) - 1,
    RUNTIME_SCENE_TRANSITION_OPTION_MASK = 0x00000fff,
    RUNTIME_SCENE_TRANSITION_PRELOAD = 0x10000000
};

enum AsyncFileHostFlag : uint32_t
{
    ASYNC_FILE_HOST_SHUTDOWN_REQUESTED = 0x00000001,
    ASYNC_FILE_HOST_REPOSITION_PENDING = 0x00000010,
    ASYNC_FILE_HOST_END_REACHED = 0x00000020,
    ASYNC_FILE_HOST_LOCKED = 0x00010000
};

enum AsyncFileRecordFlag : uint32_t
{
    ASYNC_FILE_RECORD_INITIAL_READ = 0x00000001,
    ASYNC_FILE_RECORD_SHARED = 0x00000002,
    ASYNC_FILE_RECORD_PREFETCH = 0x00000010,
    ASYNC_FILE_RECORD_BUFFER_INVALID = 0x00000020,
    ASYNC_FILE_RECORD_LOCKED = 0x00010000
};
struct DisplayTraversalState;

struct RuntimeResourceConstructionPlan
{
    uint32_t flags;
    uint32_t scene_identifier;
    uint32_t scene_flags;
    int32_t x;
    int32_t y;
    RuntimeResourceSceneRole scene_role;
};

struct RuntimeMediaBackend;
struct RuntimeAnimationBackend;
struct RuntimeSoundStatus;
struct RuntimeGenericBackend;
struct RuntimeGenericResourceNode;
struct RuntimeTreeNode;
struct DisplaySceneNode;
struct DisplayPixelFormatDescriptor;
struct AsyncFileRecord;
struct RuntimeResourceCacheEntry;
struct DisplayTraversalState;

struct RuntimeResourceVisibilityCallbackContext
{
    uint32_t palette_state;
    uint32_t resource_flags;
    char resource_name[260];
};



using RuntimeGameDllExecute = void (*)(uint32_t command);
using RuntimeGameInputHandler = uint32_t (*)(const RuntimeInputEvent &event);

struct DisplayRectangle;

} // namespace freegag
