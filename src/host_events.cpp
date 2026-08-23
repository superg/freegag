#include "host_events.h"
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

namespace gag
{

class HostEventCompletion
{
public:
    std::mutex mutex;
    std::condition_variable changed;
    HostEventResult result;
    bool complete{};
};

namespace
{
struct PendingHostEvent
{
    HostEvent event;
    std::shared_ptr<HostEventCompletion> completion;
};

struct HostEventQueue
{
    std::deque<PendingHostEvent> events;
    std::mutex mutex;
    std::thread::id host_thread;
    HostEventWake wake{};
    void *wake_context{};
    HostEventHandler handler{};
    void *handler_context{};
    bool wake_pending{};
    bool open{};
};

HostEventQueue queue;

void complete_host_event(const std::shared_ptr<HostEventCompletion> &completion, HostEventResult result)
{
    if(completion == nullptr)
    {
        return;
    }
    {
        std::lock_guard lock(completion->mutex);
        if(completion->complete)
        {
            return;
        }
        completion->result = std::move(result);
        completion->complete = true;
    }
    completion->changed.notify_all();
}

HostEventResult dispatch_host_event(const HostEvent &event, HostEventCompletion *completion)
{
    HostEventHandler handler;
    void *handler_context;
    {
        std::lock_guard lock(queue.mutex);
        handler = queue.handler;
        handler_context = queue.handler_context;
    }
    if(handler == nullptr)
    {
        return {};
    }
    return handler(event, completion, handler_context);
}
} // namespace

void initialize_host_events(HostEventWake wake, void *wake_context, HostEventHandler handler, void *handler_context)
{
    close_host_events();
    std::lock_guard lock(queue.mutex);
    queue.host_thread = std::this_thread::get_id();
    queue.wake = wake;
    queue.wake_context = wake_context;
    queue.handler = handler;
    queue.handler_context = handler_context;
    queue.wake_pending = false;
    queue.open = true;
}

void close_host_events()
{
    std::deque<PendingHostEvent> pending;
    {
        std::lock_guard lock(queue.mutex);
        queue.open = false;
        queue.wake_pending = false;
        pending.swap(queue.events);
        queue.host_thread = {};
        queue.wake = nullptr;
        queue.wake_context = nullptr;
        queue.handler = nullptr;
        queue.handler_context = nullptr;
    }
    for(PendingHostEvent &event : pending)
    {
        complete_host_event(event.completion, {});
    }
}

void post_host_event(HostEvent event)
{
    HostEventWake wake;
    void *wake_context;
    {
        std::lock_guard lock(queue.mutex);
        if(!queue.open)
        {
            return;
        }
        queue.events.push_back({ std::move(event), nullptr });
        if(queue.wake_pending)
        {
            return;
        }
        queue.wake_pending = true;
        wake = queue.wake;
        wake_context = queue.wake_context;
    }
    if(wake != nullptr && !wake(wake_context))
    {
        close_host_events();
    }
}

HostEventResult send_host_event(HostEvent event)
{
    bool on_host_thread;
    {
        std::lock_guard lock(queue.mutex);
        if(!queue.open)
        {
            return {};
        }
        on_host_thread = queue.host_thread == std::this_thread::get_id();
    }
    if(on_host_thread)
    {
        return dispatch_host_event(event, nullptr);
    }

    auto completion = std::make_shared<HostEventCompletion>();
    HostEventWake wake;
    void *wake_context;
    {
        std::lock_guard lock(queue.mutex);
        if(!queue.open)
        {
            return {};
        }
        queue.events.push_back({ std::move(event), completion });
        wake = nullptr;
        wake_context = nullptr;
        if(!queue.wake_pending)
        {
            queue.wake_pending = true;
            wake = queue.wake;
            wake_context = queue.wake_context;
        }
    }
    if(wake != nullptr && !wake(wake_context))
    {
        close_host_events();
    }

    std::unique_lock lock(completion->mutex);
    completion->changed.wait(lock, [&] { return completion->complete; });
    return std::move(completion->result);
}

void post_application_event(uint32_t command, HostApplicationPayload payload)
{
    post_host_event(HostApplicationEvent{ command, std::move(payload) });
}

void post_application_event(HostApplicationCommand command, HostApplicationPayload payload)
{
    post_application_event(static_cast<uint32_t>(command), std::move(payload));
}

HostEventResult send_application_event(uint32_t command, HostApplicationPayload payload)
{
    return send_host_event(HostApplicationEvent{ command, std::move(payload) });
}

HostEventResult send_application_event(HostApplicationCommand command, HostApplicationPayload payload)
{
    return send_application_event(static_cast<uint32_t>(command), std::move(payload));
}

void acknowledge_host_event(HostEventCompletion *completion, HostEventResult result)
{
    if(completion == nullptr)
    {
        return;
    }
    {
        std::lock_guard lock(completion->mutex);
        if(completion->complete)
        {
            return;
        }
        completion->result = std::move(result);
        completion->complete = true;
    }
    completion->changed.notify_all();
}

void drain_host_events()
{
    for(;;)
    {
        PendingHostEvent pending;
        {
            std::lock_guard lock(queue.mutex);
            if(queue.host_thread != std::this_thread::get_id())
            {
                return;
            }
            if(!queue.open || queue.events.empty())
            {
                queue.wake_pending = false;
                return;
            }
            pending = std::move(queue.events.front());
            queue.events.pop_front();
        }
        HostEventResult result = dispatch_host_event(pending.event, pending.completion.get());
        complete_host_event(pending.completion, std::move(result));
    }
}

} // namespace gag
