#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace gag
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
    runtime_failure = 0x04000000,
    close_requested = 0x10000000,
    credits_finished = 0x30000000,
    state_activated = 0x40000000,
    command_completed = 0x60000000,
    runtime_shutdown = 0x90000000,
    validate_resource_path = 0xa0000000,
    extract_file_name = 0xb0000000,
    storage_failure = 0xc0000000,
    animation_failure = 0xd0000000
};

struct HostApplicationEvent
{
    uint32_t command{};
    HostApplicationPayload payload;
};

struct HostPresentPendingFramesEvent
{
};

enum class HostXtEtEventType
{
    terminate,
    pause,
    resume,
    result
};

struct HostXtEtEvent
{
    HostXtEtEventType type{};
    uint32_t result_type{};
    std::vector<uint8_t> result_data;
};

using HostEvent = std::variant<HostApplicationEvent, HostPresentPendingFramesEvent, HostXtEtEvent>;
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

} // namespace gag
