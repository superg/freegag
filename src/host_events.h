#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace freegag
{

struct RuntimeTreeNode;

struct HostStateFieldQuery
{
    std::string object_name;
    std::string field_name;
};

using HostApplicationPayload = std::variant<std::monostate, RuntimeTreeNode *, HostStateFieldQuery, std::string>;

enum class HostApplicationCommand : uint32_t
{
    INITIALIZE_RUNTIME_FLAGS = 1,
    QUERY_LOAD_DISABLED = 0x3ea,
    QUERY_SAVE_DISABLED = 0x3eb,
    QUERY_RESUME_DISABLED = 0x3ec,
    QUERY_NEW_GAME_DISABLED = 0x3ed,
    QUERY_FULLSCREEN_ENABLED = 0x3f2,
    QUERY_SOUND_MUTED = 0x3fc,
    QUERY_SUBTITLES_ENABLED = 0x406,
    QUERY_FULLSCREEN_UNAVAILABLE = 0x456,
    START_NEW_GAME = 0x7d1,
    OPEN_LOAD_SCREEN = 0x7d2,
    OPEN_SAVE_SCREEN = 0x7d3,
    RESUME_SAVED_GAME = 0x7d4,
    TOGGLE_FULLSCREEN = 0x7da,
    TOGGLE_MUTE = 0x7e4,
    TOGGLE_COMMENTS = 0x7ee,
    CAPTURE_STATE_SNAPSHOT = 0xbc2,
    RELEASE_STATE_SNAPSHOT = 0xbcc,
    QUERY_PREFERENCES_CHANGED = 0xbd6,
    RUNTIME_FAILURE = 0x04000000,
    CLOSE_REQUESTED = 0x10000000,
    CREDITS_FINISHED = 0x30000000,
    STATE_ACTIVATED = 0x40000000,
    COMMAND_COMPLETED = 0x60000000,
    RUNTIME_SHUTDOWN = 0x90000000,
    VALIDATE_RESOURCE_PATH = 0xa0000000,
    EXTRACT_FILE_NAME = 0xb0000000,
    STORAGE_FAILURE = 0xc0000000,
    ANIMATION_FAILURE = 0xd0000000
};

struct HostApplicationEvent
{
    uint32_t command{};
    HostApplicationPayload payload;
};

struct HostPresentPendingFramesEvent
{
};

struct HostKeyboardInputDrainEvent
{
};

enum class HostXtEtEventType
{
    TERMINATE,
    PAUSE,
    RESUME,
    RESULT
};

struct HostXtEtEvent
{
    HostXtEtEventType type{};
    uint32_t result_type{};
    std::vector<uint8_t> result_data;
};

using HostEvent = std::variant<HostApplicationEvent, HostPresentPendingFramesEvent, HostKeyboardInputDrainEvent, HostXtEtEvent>;
using HostEventResult = std::variant<std::monostate, uint32_t, std::string>;

class HostEventCompletion;

using HostEventWake = bool (*)(void *context);
using HostEventHandler = HostEventResult (*)(const HostEvent &event, HostEventCompletion *completion, void *context);

void initialize_host_events(HostEventWake wake, void *wake_context, HostEventHandler handler, void *handler_context);

void close_host_events();

void post_host_event(HostEvent event);

HostEventResult send_host_event(HostEvent event);

void post_application_event(uint32_t command, HostApplicationPayload payload = {});

void post_application_event(HostApplicationCommand command, HostApplicationPayload payload = {});

HostEventResult send_application_event(uint32_t command, HostApplicationPayload payload = {});

HostEventResult send_application_event(HostApplicationCommand command, HostApplicationPayload payload = {});

void acknowledge_host_event(HostEventCompletion *completion, HostEventResult result = uint32_t{});

void drain_host_events();

} // namespace freegag
