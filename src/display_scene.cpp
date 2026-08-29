#include "display_scene.h"
#include <new>
#include "runtime_internal.h"

namespace freegag
{

uint32_t acquire_display_lock(DisplayRectangle *primary_rectangle, DisplayRectangle *secondary_rectangle, uint32_t *rectangle_flags)
{
    if((display_lock_flags & DISPLAY_SCENE_HOST_INITIALIZED) == 0)
        return DISPLAY_OPERATION_FAILED;
    uint32_t busy = 0;
    uint32_t mode = 0;
    RuntimeThreadId thread_id = runtime_thread_id();
    do
    {
        do
        {
            if(busy != 0)
                wait_runtime_event(display_lock_gate_event);
            if(mode == DISPLAY_SCENE_LOCK_MODE_MASK)
                wait_runtime_event(display_lock_release_event);
            if(mode == DISPLAY_SCENE_LOCK_ACQUIRED)
                runtime_sleep(5);
            lock_runtime_mutex(display_lock_mutex);
            busy = display_lock_busy;
            mode = display_lock_flags & DISPLAY_SCENE_LOCK_MODE_MASK;
            if((mode == 0 || display_lock_owner_thread == thread_id) && busy == 0)
            {
                uint32_t dirty_flags = 0;
                bool first_acquisition = display_lock_recursion_count == 0;
                ++display_lock_recursion_count;
                if(first_acquisition || mode == DISPLAY_SCENE_LOCK_ACQUIRED)
                {
                    display_lock_release_event->reset();
                    display_lock_flags |= DISPLAY_SCENE_LOCK_MODE_MASK;
                    display_lock_owner_thread = thread_id;
                }
                mode = 0;
                if(primary_rectangle != nullptr)
                {
                    *primary_rectangle = display_pending_rectangle;
                    display_pending_rectangle.right = 0;
                    display_pending_rectangle.bottom = 0;
                    display_pending_rectangle.left = display_width;
                    display_pending_rectangle.top = display_height;
                    for(DisplaySceneNode *node = display_scene_head; node != nullptr; node = node->next)
                    {
                        if(node->state_60 != 0)
                            process_scene_node_callbacks(node);
                        accumulate_scene_node_rectangle(primary_rectangle, node);
                    }
                    if(clip_display_rectangle(primary_rectangle))
                        dirty_flags = DISPLAY_DIRTY_PRIMARY;
                    if(secondary_rectangle != nullptr)
                    {
                        *secondary_rectangle = *primary_rectangle;
                        for(DisplaySceneNode *node = display_scene_head; node != nullptr; node = node->next)
                            if((node->flags & DISPLAY_SCENE_OPAQUE) != 0)
                                trim_display_rectangle_overlap(secondary_rectangle, node);
                        if(clip_display_rectangle(secondary_rectangle))
                            dirty_flags |= DISPLAY_DIRTY_SECONDARY;
                    }
                }
                if(rectangle_flags != nullptr)
                    *rectangle_flags = dirty_flags;
            }
            unlock_runtime_mutex(display_lock_mutex);
        } while(busy != 0);
    } while((mode & 0xff00) != 0);
    return mode;
}

uint32_t release_display_lock()
{
    uint32_t result = DISPLAY_OPERATION_FAILED;
    if((display_lock_flags & DISPLAY_SCENE_HOST_INITIALIZED) != 0)
    {
        RuntimeThreadId thread_id = runtime_thread_id();
        if(thread_id == display_lock_owner_thread && (display_lock_flags & DISPLAY_SCENE_LOCK_MODE_MASK) != 0)
        {
            result = DISPLAY_OPERATION_SUCCESS;
            --display_lock_recursion_count;
            if(display_lock_recursion_count == 0)
            {
                if((display_lock_flags & DISPLAY_SCENE_LOCK_RELEASE_PENDING) != 0)
                    set_runtime_event(display_lock_release_event);
                display_lock_owner_thread = {};
                display_lock_flags &= ~DISPLAY_SCENE_LOCK_MODE_MASK;
            }
        }
    }
    return result;
}

bool clip_display_rectangle(DisplayRectangle *rectangle)
{
    if(rectangle->left < display_clip_bounds.left)
        rectangle->left = display_clip_bounds.left;
    if(rectangle->top < display_clip_bounds.top)
        rectangle->top = display_clip_bounds.top;
    if(display_clip_bounds.right < rectangle->right)
        rectangle->right = display_clip_bounds.right;
    if(display_clip_bounds.bottom < rectangle->bottom)
        rectangle->bottom = display_clip_bounds.bottom;
    return rectangle->right != rectangle->left && rectangle->left <= rectangle->right && rectangle->bottom != rectangle->top && rectangle->top <= rectangle->bottom;
}

bool constrain_display_rectangle_to_surface(DisplayRectangle *rectangle)
{
    if(rectangle->left < 0)
        rectangle->left = 0;
    if(rectangle->top < 0)
        rectangle->top = 0;
    if(display_width < rectangle->right)
        rectangle->right = display_width;
    if(display_height < rectangle->bottom)
        rectangle->bottom = display_height;
    return rectangle->right != rectangle->left && rectangle->left <= rectangle->right && rectangle->bottom != rectangle->top && rectangle->top <= rectangle->bottom;
}

int process_scene_node_callbacks(DisplaySceneNode *node)
{
    DisplayTraversalState state{ DISPLAY_TRAVERSAL_QUERY, runtime_milliseconds(), static_cast<uint32_t>(node->width), static_cast<uint32_t>(node->height), node->callback_first_position,
        node->callback_current_position, &node->accumulated_rectangle.left, &display_clip_bounds, nullptr };
    int result = DISPLAY_TRAVERSAL_UNCHANGED;
    for(DisplaySceneCallbackNode *callback = node->callbacks; callback != nullptr; callback = callback->next)
    {
        state.callback_context = callback->context;
        if(callback->callback(&state) == DISPLAY_TRAVERSAL_BUFFER_UPDATED)
            result = DISPLAY_TRAVERSAL_BUFFER_UPDATED;
    }
    if(result == DISPLAY_TRAVERSAL_BUFFER_UPDATED)
    {
        state.flags = DISPLAY_TRAVERSAL_RENDER;
        for(DisplaySceneCallbackNode *callback = node->callbacks; callback != nullptr; callback = callback->next)
        {
            state.callback_context = callback->context;
            result = callback->callback(&state);
            if(result == DISPLAY_TRAVERSAL_STOP)
                break;
            if(result == DISPLAY_TRAVERSAL_BUFFER_UPDATED && (callback->flags & DISPLAY_SCENE_CALLBACK_NO_BUFFER_SWAP) == 0)
            {
                state.first_position = state.current_position;
                if(state.current_position == node->callback_current_position)
                    state.current_position = node->callback_alternate_position;
                else
                    state.current_position = node->callback_current_position;
            }
        }
        node->callback_position = state.first_position;
    }
    return result;
}

void trim_display_rectangle_overlap(DisplayRectangle *rectangle, DisplaySceneNode *node)
{
    if(node == nullptr || rectangle == nullptr || (node->flags & DISPLAY_SCENE_UPDATE_PENDING) != 0)
        return;
    int32_t node_left = node->x;
    int32_t node_top = node->y;
    int32_t node_right = node_left + node->width;
    int32_t node_bottom = node_top + node->height;
    int32_t overlap_left = node_left;
    uint32_t edges = 0;
    if(node_left <= rectangle->left)
    {
        overlap_left = rectangle->left;
        edges = 1;
    }
    if(node_top <= rectangle->top)
    {
        node_top = rectangle->top;
        edges |= 0x10000;
    }
    if(rectangle->right <= node_right)
    {
        node_right = rectangle->right;
        edges |= 2;
    }
    if(rectangle->bottom <= node_bottom)
    {
        node_bottom = rectangle->bottom;
        edges |= 0x20000;
    }
    int32_t overlap_width = node_right - overlap_left;
    int32_t overlap_height = node_bottom - node_top;
    if(overlap_width != 0 && overlap_left <= node_right && overlap_height != 0 && node_top <= node_bottom)
    {
        if((edges & 3) == 3)
        {
            if((edges & 0x10000) != 0)
                rectangle->top += overlap_height;
            if((edges & 0x20000) != 0)
                rectangle->bottom -= overlap_height;
        }
        if((edges & 0x30000) == 0x30000)
        {
            if((edges & 1) != 0)
                rectangle->left += overlap_width;
            if((edges & 2) != 0)
                rectangle->right -= overlap_width;
        }
    }
}

void accumulate_scene_node_rectangle(DisplayRectangle *rectangle, DisplaySceneNode *node)
{
    if(rectangle == nullptr || node == nullptr)
        return;
    int32_t left = node->x + node->x_offset;
    int32_t top = node->y + node->y_offset;
    int32_t right;
    int32_t bottom;
    if(left == node->previous_x && top == node->previous_y)
    {
        right = left + node->accumulated_rectangle.right;
        bottom = top + node->accumulated_rectangle.bottom;
        left += node->accumulated_rectangle.left;
        top += node->accumulated_rectangle.top;
    }
    else
    {
        node->x_offset = 0;
        node->y_offset = 0;
        node->x = left;
        node->y = top;
        right = node->previous_x;
        bottom = node->previous_y;
        if(right < left)
        {
            int32_t swap = left;
            left = right;
            right = swap;
        }
        if(bottom < top)
        {
            int32_t swap = top;
            top = bottom;
            bottom = swap;
        }
        right += node->width;
        bottom += node->height;
    }
    if(left < 0)
        left = 0;
    if(top < 0)
        top = 0;
    if(node->surface->width < right)
        right = node->surface->width;
    if(node->surface->height < bottom)
        bottom = node->surface->height;
    if(left < right && top < bottom)
    {
        if(left < rectangle->left)
            rectangle->left = left;
        if(top < rectangle->top)
            rectangle->top = top;
        if(rectangle->right < right)
            rectangle->right = right;
        if(rectangle->bottom < bottom)
            rectangle->bottom = bottom;
    }
    node->accumulated_rectangle.right = 0;
    node->accumulated_rectangle.bottom = 0;
    node->accumulated_rectangle.left = node->width;
    node->accumulated_rectangle.top = node->height;
    node->previous_x = node->x;
    node->previous_y = node->y;
}

void merge_display_rectangle(DisplayRectangle *destination, const DisplayRectangleTransform *transform, const DisplayRectangle *source)
{
    if(source == nullptr || destination == nullptr)
        return;
    int32_t left = source->left;
    int32_t top = source->top;
    int32_t right = source->right;
    int32_t bottom = source->bottom;
    if(transform != nullptr)
    {
        left = static_cast<int32_t>((static_cast<uint32_t>(left) & 0xffff0000) | static_cast<uint16_t>(static_cast<int16_t>(left) + transform->x));
        right = static_cast<int32_t>((static_cast<uint32_t>(right) & 0xffff0000) | static_cast<uint16_t>(static_cast<int16_t>(right) + transform->x));
        top = static_cast<int32_t>((static_cast<uint32_t>(top) & 0xffff0000) | static_cast<uint16_t>(static_cast<int16_t>(top) + transform->y));
        bottom = static_cast<int32_t>((static_cast<uint32_t>(bottom) & 0xffff0000) | static_cast<uint16_t>(static_cast<int16_t>(bottom) + transform->y));
    }
    if(left < destination->left)
        destination->left = left;
    if(top < destination->top)
        destination->top = top;
    if(destination->right < right)
        destination->right = right;
    if(destination->bottom < bottom)
        destination->bottom = bottom;
    if(transform != nullptr)
    {
        if(destination->left < 0)
            destination->left = 0;
        if(destination->top < 0)
            destination->top = 0;
        if(static_cast<int32_t>(transform->width) < destination->right)
            destination->right = transform->width;
        if(static_cast<int32_t>(transform->height) < destination->bottom)
            destination->bottom = transform->height;
    }
}

uint32_t queue_display_rectangle(DisplayRectangle *rectangle)
{
    if((display_lock_flags & DISPLAY_SCENE_HOST_INITIALIZED) == 0)
        return DISPLAY_OPERATION_FAILED;
    if(!constrain_display_rectangle_to_surface(rectangle))
        return DISPLAY_OPERATION_SUCCESS;
    lock_runtime_mutex(display_lock_mutex);
    merge_display_rectangle(&display_pending_rectangle, nullptr, rectangle);
    unlock_runtime_mutex(display_lock_mutex);
    return DISPLAY_OPERATION_SUCCESS;
}

uint32_t find_available_display_scene_index(uint32_t candidate)
{
    if((display_lock_flags & DISPLAY_SCENE_HOST_INITIALIZED) == 0)
        return candidate;
    lock_runtime_mutex(display_lock_mutex);
    DisplaySceneNode *current = display_scene_head;
    DisplaySceneNode *previous = display_scene_head;
    while(current != nullptr && ((candidate <= previous->scene_index && previous != current) || current->scene_index <= candidate))
    {
        uint32_t current_index = current->scene_index;
        uint32_t next_candidate = candidate;
        if(candidate <= current_index && current_index <= candidate)
            next_candidate = candidate + 1;
        candidate = next_candidate;
        previous = current;
        current = current->next;
    }
    unlock_runtime_mutex(display_lock_mutex);
    return candidate;
}

uint32_t wait_for_display_scene_ready(uint32_t timeout)
{
    if((display_lock_flags & DISPLAY_SCENE_HOST_INITIALIZED) != 0)
    {
        display_lock_flags &= ~DISPLAY_SCENE_WORKER_READY;
        uint32_t start = runtime_milliseconds();
        do
        {
            if((display_lock_flags & DISPLAY_SCENE_WORKER_READY) != 0)
                return DISPLAY_OPERATION_SUCCESS;
            runtime_sleep(0);
        } while(runtime_milliseconds() - start <= timeout);
    }
    return DISPLAY_OPERATION_FAILED;
}

uint32_t set_display_clip_rectangle(DisplayRectangle *rectangle)
{
    if((display_lock_flags & DISPLAY_SCENE_HOST_INITIALIZED) == 0)
        return DISPLAY_OPERATION_FAILED;
    RuntimeThreadId thread_id = runtime_thread_id();
    if((display_lock_flags & DISPLAY_SCENE_LOCK_ACQUIRED) == 0 || display_lock_owner_thread != thread_id)
        return DISPLAY_OPERATION_LOCK_NOT_OWNED;
    if(rectangle == nullptr)
    {
        display_clip_bounds = { 0, 0, 0, 0 };
        return DISPLAY_OPERATION_SUCCESS;
    }
    if(rectangle->right < 0 || rectangle->bottom < 0 || rectangle->left > display_width || rectangle->top > display_height)
        return DISPLAY_OPERATION_FAILED;
    constrain_display_rectangle_to_surface(rectangle);
    display_clip_bounds = *rectangle;
    return DISPLAY_OPERATION_SUCCESS;
}

uint32_t release_pending_display_lock()
{
    if((display_lock_flags & DISPLAY_SCENE_HOST_INITIALIZED) == 0)
        return DISPLAY_OPERATION_FAILED;
    RuntimeThreadId thread_id = runtime_thread_id();
    if(display_lock_owner_thread != thread_id || (display_lock_flags & DISPLAY_SCENE_LOCK_RELEASE_PENDING) == 0)
        return DISPLAY_OPERATION_FAILED;
    if(display_lock_recursion_count != 1)
        return DISPLAY_OPERATION_RELEASE_PENDING;
    display_lock_flags &= ~DISPLAY_SCENE_LOCK_RELEASE_PENDING;
    set_runtime_event(display_lock_release_event);
    return DISPLAY_OPERATION_SUCCESS;
}

DisplaySceneNode *lock_display_scene_node(intptr_t identifier)
{
    if((display_lock_flags & DISPLAY_SCENE_HOST_INITIALIZED) == 0)
        return nullptr;
    RuntimeThreadId thread_id = runtime_thread_id();
    while(true)
    {
        bool busy = false;
        DisplaySceneNode *result = nullptr;
        lock_runtime_mutex(display_lock_mutex);
        for(DisplaySceneNode *node = display_scene_head; node != nullptr; node = node->next)
        {
            if(node->identifier == identifier)
            {
                if(node->lock_count == 0 || node->lock_owner_thread == thread_id)
                {
                    ++node->lock_count;
                    node->lock_owner_thread = thread_id;
                    result = node;
                }
                else
                {
                    busy = true;
                }
                break;
            }
        }
        unlock_runtime_mutex(display_lock_mutex);
        if(!busy)
            return result;
        runtime_sleep(5);
    }
}

void unlock_display_scene_node(intptr_t identifier)
{
    if((display_lock_flags & DISPLAY_SCENE_HOST_INITIALIZED) == 0)
        return;
    RuntimeThreadId thread_id = runtime_thread_id();
    lock_runtime_mutex(display_lock_mutex);
    for(DisplaySceneNode *node = display_scene_head; node != nullptr; node = node->next)
    {
        if(node->identifier == identifier)
        {
            if(node->lock_count != 0 && node->lock_owner_thread == thread_id)
            {
                --node->lock_count;
                if(node->lock_count == 0)
                    node->lock_owner_thread = {};
            }
            break;
        }
    }
    unlock_runtime_mutex(display_lock_mutex);
}

bool set_display_scene_primary_owner(intptr_t identifier, intptr_t owner, bool replace_existing)
{
    if((display_lock_flags & DISPLAY_SCENE_HOST_INITIALIZED) == 0)
        return false;
    bool result = false;
    lock_runtime_mutex(display_lock_mutex);
    for(DisplaySceneNode *node = display_scene_head; node != nullptr; node = node->next)
    {
        if(node->identifier == identifier)
        {
            if(replace_existing || node->primary_owner == 0)
            {
                if(owner == 0)
                {
                    node->primary_owner = 0;
                    result = true;
                }
                else
                {
                    for(uint32_t index = 0; index < node->owner_count; ++index)
                    {
                        if(node->owners[index] == owner)
                        {
                            node->primary_owner = owner;
                            result = true;
                            break;
                        }
                    }
                }
            }
            break;
        }
    }
    unlock_runtime_mutex(display_lock_mutex);
    return result;
}

intptr_t query_display_scene_by_index(int32_t index, DisplaySceneDescriptor *descriptor, DisplayPixelFormatDescriptor *callback_format)
{
    if((display_lock_flags & DISPLAY_SCENE_HOST_INITIALIZED) == 0)
        return 0;
    intptr_t result = 0;
    lock_runtime_mutex(display_lock_mutex);
    DisplaySceneNode *selected = nullptr;
    for(DisplaySceneNode *node = display_scene_head; node != nullptr; node = node->next)
    {
        if(static_cast<int32_t>(node->scene_index) == index)
        {
            if(selected == nullptr || node->storage == DisplaySceneStorage::XRGB_COMPOSITION)
                selected = node;
            if(node->storage == DisplaySceneStorage::XRGB_COMPOSITION)
                break;
        }
    }
    if(selected != nullptr)
    {
        result = selected->identifier;
        if(descriptor != nullptr)
        {
            descriptor->x = 0;
            descriptor->y = 0;
            descriptor->width = static_cast<int16_t>(selected->width);
            descriptor->height = static_cast<int16_t>(selected->height);
            descriptor->pixels = selected->callback_first_position;
            descriptor->bits_per_pixel = selected->rectangle_callback_format.bits_per_pixel;
            descriptor->stride = static_cast<uint32_t>(selected->sync_secondary_position) / (selected->rectangle_callback_format.bits_per_pixel >> 3);
        }
        if(callback_format != nullptr)
            *callback_format = selected->rectangle_callback_format;
    }
    unlock_runtime_mutex(display_lock_mutex);
    if(result == 0)
    {
        if(descriptor != nullptr)
            std::memset(descriptor, 0, sizeof(*descriptor));
        if(callback_format != nullptr)
            *callback_format = {};
    }
    return result;
}

uint32_t blit_display_scene(DisplaySceneNode *destination, int32_t destination_x, int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle, uint32_t flags)
{
    if(destination == nullptr || source == nullptr || destination->storage != DisplaySceneStorage::XRGB_COMPOSITION)
        return DISPLAY_OPERATION_FAILED;
    uint32_t result = begin_display_scene_update(reinterpret_cast<intptr_t>(source));
    if(result == DISPLAY_OPERATION_SUCCESS)
    {
        result = begin_display_scene_update(reinterpret_cast<intptr_t>(destination));
        if(result == DISPLAY_OPERATION_SUCCESS)
        {
            DisplayRectangleTransform transform{};
            transform.width = static_cast<uint16_t>(destination->width);
            transform.height = static_cast<uint16_t>(destination->height);

            if(source->storage == DisplaySceneStorage::INDEXED_SOURCE && source->rectangle_callback_format.palette_source != nullptr)
            {
                if(source->rectangle_callback_format.palette_entries == nullptr)
                {
                    source->rectangle_callback_format.palette_entries = source->palette_mapping;
                    build_indexed_to_32_palette(&source->rectangle_callback_format);
                }
                if((flags & DISPLAY_SCENE_FIXED_SIZE) == 0)
                    composite_opaque_indexed_to_32(destination, destination_x, destination_y, source, rectangle, &source->rectangle_callback_format, 0);
                else
                    composite_transparent_indexed_to_32(destination, destination_x, destination_y, source, rectangle, &source->rectangle_callback_format, 0);
            }
            else if(source->storage == DisplaySceneStorage::XRGB_COMPOSITION)
            {
                if((flags & DISPLAY_SCENE_FIXED_SIZE) == 0)
                    composite_opaque_32_to_32(destination, destination_x, destination_y, source, rectangle, nullptr, 0);
                else
                    composite_transparent_32_to_32(destination, destination_x, destination_y, source, rectangle, nullptr, 0);
            }
            end_display_scene_update(reinterpret_cast<intptr_t>(destination), &transform, rectangle);
        }
        end_display_scene_update(reinterpret_cast<intptr_t>(source), nullptr, nullptr);
    }
    return result;
}

uint32_t offset_display_scene_node(intptr_t identifier, int32_t x_delta, int32_t y_delta)
{
    if((display_lock_flags & DISPLAY_SCENE_HOST_INITIALIZED) == 0)
        return DISPLAY_OPERATION_FAILED;
    uint32_t result = DISPLAY_OPERATION_FAILED;
    lock_runtime_mutex(display_lock_mutex);
    for(DisplaySceneNode *node = display_scene_head; node != nullptr; node = node->next)
    {
        if(node->identifier == identifier)
        {
            if((node->flags & (DISPLAY_SCENE_FIXED_POSITION | DISPLAY_SCENE_STATIC)) == 0)
            {
                node->x_offset += x_delta;
                node->y_offset += y_delta;
            }
            result = DISPLAY_OPERATION_SUCCESS;
            break;
        }
    }
    unlock_runtime_mutex(display_lock_mutex);
    return result;
}

uint32_t set_display_scene_node_position(intptr_t identifier, int32_t x, int32_t y)
{
    if((display_lock_flags & DISPLAY_SCENE_HOST_INITIALIZED) == 0)
        return DISPLAY_OPERATION_FAILED;
    uint32_t result = DISPLAY_OPERATION_FAILED;
    lock_runtime_mutex(display_lock_mutex);
    for(DisplaySceneNode *node = display_scene_head; node != nullptr; node = node->next)
    {
        if(node->identifier == identifier)
        {
            if((node->flags & (DISPLAY_SCENE_FIXED_POSITION | DISPLAY_SCENE_STATIC)) == 0)
            {
                node->x_offset = x - node->x;
                node->y_offset = y - node->y;
            }
            result = DISPLAY_OPERATION_SUCCESS;
            break;
        }
    }
    unlock_runtime_mutex(display_lock_mutex);
    return result;
}

uint32_t begin_display_scene_update(intptr_t identifier)
{
    if((display_lock_flags & DISPLAY_SCENE_HOST_INITIALIZED) == 0)
        return DISPLAY_OPERATION_FAILED;
    while(true)
    {
        lock_runtime_mutex(display_lock_mutex);
        uint32_t result = display_lock_flags & DISPLAY_SCENE_LOCK_RELEASE_PENDING;
        if(result == DISPLAY_OPERATION_SUCCESS)
        {
            result = DISPLAY_OPERATION_FAILED;
            for(DisplaySceneNode *node = display_scene_head; node != nullptr; node = node->next)
            {
                if(node->identifier == identifier)
                {
                    if(node->owner_count != 0 && (node->flags & DISPLAY_SCENE_UPDATE_PENDING) != 0)
                    {
                        node->flags &= ~DISPLAY_SCENE_UPDATE_PENDING;
                        node->accumulated_rectangle.left = 0;
                        node->accumulated_rectangle.top = 0;
                        node->accumulated_rectangle.right = node->width;
                        node->accumulated_rectangle.bottom = node->height;
                    }
                    if(display_lock_busy == 0)
                        display_lock_gate_event->reset();
                    ++display_lock_busy;
                    result = DISPLAY_OPERATION_SUCCESS;
                    break;
                }
            }
        }
        unlock_runtime_mutex(display_lock_mutex);
        if(result != DISPLAY_OPERATION_RELEASE_PENDING)
            return result;
        wait_runtime_event(display_lock_release_event);
    }
}

bool activate_display_scene_node(intptr_t identifier)
{
    if((display_lock_flags & DISPLAY_SCENE_HOST_INITIALIZED) == 0)
        return false;
    bool activated = false;
    lock_runtime_mutex(display_lock_mutex);
    for(DisplaySceneNode *node = display_scene_head; node != nullptr; node = node->next)
    {
        if(node->identifier == identifier)
        {
            node->flags &= ~DISPLAY_SCENE_UPDATE_PENDING;
            node->accumulated_rectangle = { 0, 0, node->width, node->height };
            activated = true;
            break;
        }
    }
    unlock_runtime_mutex(display_lock_mutex);
    return activated;
}

uint32_t end_display_scene_update(intptr_t identifier, const DisplayRectangleTransform *transform, const DisplayRectangle *rectangle)
{
    if((display_lock_flags & DISPLAY_SCENE_HOST_INITIALIZED) == 0)
        return DISPLAY_OPERATION_FAILED;
    uint32_t result = DISPLAY_OPERATION_FAILED;
    lock_runtime_mutex(display_lock_mutex);
    for(DisplaySceneNode *node = display_scene_head; node != nullptr; node = node->next)
    {
        if(node->identifier == identifier)
        {
            if(display_lock_busy != 0)
            {
                merge_display_rectangle(&node->accumulated_rectangle, transform, rectangle);
                --display_lock_busy;
                if(display_lock_busy == 0)
                    set_runtime_event(display_lock_gate_event);
                result = DISPLAY_OPERATION_SUCCESS;
            }
            break;
        }
    }
    unlock_runtime_mutex(display_lock_mutex);
    return result;
}

uint32_t update_display_root_region(DisplaySceneNode *scene, DisplayRectangle *rectangle, uint32_t callback_value)
{
    if((display_lock_flags & DISPLAY_SCENE_HOST_INITIALIZED) == 0)
        return DISPLAY_OPERATION_FAILED;
    if(scene == nullptr)
        scene = display_scene_root;
    uint32_t result = begin_display_scene_update(reinterpret_cast<intptr_t>(scene));
    if(result == DISPLAY_OPERATION_SUCCESS)
    {
        DisplayRectangleTransform transform{ 0, 0, static_cast<uint16_t>(scene->width), static_cast<uint16_t>(scene->height) };
        if(scene->root_rectangle_callback != nullptr)
            scene->root_rectangle_callback(scene, rectangle, callback_value);
        end_display_scene_update(reinterpret_cast<intptr_t>(scene), &transform, rectangle);
    }
    return result;
}

uint32_t add_display_scene_callback(intptr_t identifier, int (*callback)(DisplayTraversalState *state), const void *context, uint32_t context_size, uint32_t flags)
{
    if((display_lock_flags & DISPLAY_SCENE_HOST_INITIALIZED) == 0)
        return DISPLAY_OPERATION_FAILED;
    while(true)
    {
        lock_runtime_mutex(display_lock_mutex);
        uint32_t result = display_lock_flags & DISPLAY_SCENE_LOCK_RELEASE_PENDING;
        if(result == DISPLAY_OPERATION_SUCCESS)
        {
            result = DISPLAY_OPERATION_FAILED;
            DisplaySceneNode *node = display_scene_head;
            while(node != nullptr && node->identifier != identifier)
                node = node->next;
            if(node != nullptr)
            {
                DisplaySceneCallbackNode *previous = node->callbacks;
                for(DisplaySceneCallbackNode *entry = node->callbacks; entry != nullptr; entry = entry->next)
                    previous = entry;
                RuntimeHeap *heap = runtime_process_heap();
                auto *entry = static_cast<DisplaySceneCallbackNode *>(allocate_runtime_heap(heap, 0, context_size + sizeof(DisplaySceneCallbackNode)));
                if(entry != nullptr)
                {
                    entry->identity = reinterpret_cast<uintptr_t>(entry);
                    entry->next = nullptr;
                    entry->flags = flags;
                    entry->callback = callback;
                    if(context == nullptr)
                    {
                        entry->context = nullptr;
                    }
                    else
                    {
                        entry->context = entry + 1;
                        std::memcpy(entry->context, context, context_size);
                    }
                    ++node->state_60;
                    result = DISPLAY_OPERATION_SUCCESS;
                    if((flags & DISPLAY_SCENE_CALLBACK_NO_BUFFER_SWAP) == 0)
                    {
                        intptr_t *buffer_position;
                        if(node->callback_current_position == 0)
                            buffer_position = &node->callback_current_position;
                        else if(node->callback_alternate_position == 0)
                            buffer_position = &node->callback_alternate_position;
                        else
                            buffer_position = nullptr;
                        if(buffer_position != nullptr)
                        {
                            const size_t pixel_bytes = static_cast<size_t>(node->rectangle_callback_format.bits_per_pixel >> 3);
                            void *buffer = allocate_runtime_heap(heap, 0, static_cast<size_t>(node->width) * static_cast<size_t>(node->height) * pixel_bytes);
                            *buffer_position = reinterpret_cast<intptr_t>(buffer);
                            if(buffer == nullptr)
                            {
                                --node->state_60;
                                free_runtime_heap(heap, 0, entry);
                                result = DISPLAY_OPERATION_FAILED;
                            }
                        }
                    }
                    if(result == DISPLAY_OPERATION_SUCCESS)
                    {
                        if(previous == nullptr)
                            node->callbacks = entry;
                        else
                            previous->next = entry;
                    }
                }
            }
        }
        unlock_runtime_mutex(display_lock_mutex);
        if(result != DISPLAY_OPERATION_RELEASE_PENDING)
            return result;
        wait_runtime_event(display_lock_release_event);
    }
}

void fill_display_scene_rectangle_8(DisplaySceneNode *node, DisplayRectangle *rectangle, int value)
{
    if(rectangle == nullptr || node == nullptr)
        return;
    int32_t left = rectangle->left;
    int32_t top = rectangle->top;
    int32_t right = rectangle->right;
    int32_t bottom = rectangle->bottom;
    if(left < 0)
        left = 0;
    if(top < 0)
        top = 0;
    if(node->width < right)
        right = node->width;
    if(node->height < bottom)
        bottom = node->height;
    int32_t row_width = right - left;
    int32_t row_count = bottom - top;
    if(row_width != 0 && left <= right && row_count != 0 && top <= bottom)
    {
        auto *row = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(node->callback_first_position)) + top * node->sync_secondary_position + left;
        do
        {
            std::memset(row, static_cast<uint8_t>(value), row_width);
            row += node->sync_secondary_position;
            --row_count;
        } while(row_count != 0);
    }
}


struct CompositeRegion
{
    int32_t source_x;
    int32_t source_y;
    int32_t destination_x;
    int32_t destination_y;
    int32_t width;
    int32_t height;
};

// Common control flow shared by the compositor entry points.
bool prepare_composite_region(DisplaySceneNode *destination, int32_t destination_x, int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle, uint32_t mode,
    CompositeRegion &region)
{
    if(source == nullptr || destination == nullptr || rectangle == nullptr || (source->flags & DISPLAY_SCENE_DISABLED) != 0 || (source->flags & DISPLAY_SCENE_UPDATE_PENDING) != 0)
        return false;
    int32_t source_x = 0;
    int32_t source_y = 0;
    int32_t destination_right;
    int32_t destination_bottom;
    if((mode & 0x01000000) == 0)
    {
        source_x = rectangle->left;
        source_y = rectangle->top;
        int32_t source_right = rectangle->right;
        int32_t source_bottom = rectangle->bottom;
        if(source_x < 0)
            source_x = 0;
        if(source_y < 0)
            source_y = 0;
        if(source->width < source_right)
            source_right = source->width;
        if(source->height < source_bottom)
            source_bottom = source->height;
        destination_right = (source_right - source_x) + destination_x;
        destination_bottom = (source_bottom - source_y) + destination_y;
        if(destination_x < 0)
        {
            source_x -= destination_x;
            destination_x = 0;
        }
        if(destination_y < 0)
        {
            source_y -= destination_y;
            destination_y = 0;
        }
        if(destination->width < destination_right)
            destination_right = destination->width;
        if(destination->height < destination_bottom)
            destination_bottom = destination->height;
        rectangle->left = destination_x;
        rectangle->top = destination_y;
        rectangle->right = destination_right;
        rectangle->bottom = destination_bottom;
    }
    else
    {
        int32_t source_left = source->x;
        int32_t source_top = source->y;
        int32_t source_right = source_left + source->width;
        int32_t source_bottom = source_top + source->height;
        if(source_left < rectangle->left)
        {
            source_x = rectangle->left - source_left;
            source_left = rectangle->left;
        }
        if(source_top < rectangle->top)
        {
            source_y = rectangle->top - source_top;
            source_top = rectangle->top;
        }
        if(rectangle->right < source_right)
            source_right = rectangle->right;
        if(rectangle->bottom < source_bottom)
            source_bottom = rectangle->bottom;
        destination_x = source_left - destination->x;
        if(destination_x < 0)
        {
            source_x -= destination_x;
            destination_x = 0;
        }
        destination_y = source_top - destination->y;
        if(destination_y < 0)
        {
            source_y -= destination_y;
            destination_y = 0;
        }
        destination_right = source_right - destination->x;
        destination_bottom = source_bottom - destination->y;
        if(destination->width < destination_right)
            destination_right = destination->width;
        if(destination->height < destination_bottom)
            destination_bottom = destination->height;
    }
    region = { source_x, source_y, destination_x, destination_y, destination_right - destination_x, destination_bottom - destination_y };
    return region.width != 0 && destination_x <= destination_right && region.height != 0 && destination_y <= destination_bottom;
}

// Pixel conversion selected by each compositor entry point.
void composite_indexed_pixels(DisplaySceneNode *destination, DisplaySceneNode *source, void *source_state, const CompositeRegion &region, bool transparent, bool convert_palette,
    uint32_t destination_bits)
{
    auto *source_row = reinterpret_cast<const uint8_t *>(static_cast<uintptr_t>(source->callback_position)) + region.source_y * source->sync_secondary_position + region.source_x;
    auto *destination_row = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(destination->callback_position)) + region.destination_y * destination->sync_secondary_position
                          + region.destination_x * (destination_bits >> 3);
    const uint32_t *palette = nullptr;
    if(convert_palette)
        palette = static_cast<const DisplayPixelFormatDescriptor *>(source_state)->palette_entries;
    for(int32_t row_index = 0; row_index < region.height; ++row_index)
    {
        if(destination_bits == 32)
        {
            auto *destination_pixels = reinterpret_cast<uint32_t *>(destination_row);
            for(int32_t column = 0; column < region.width; ++column)
            {
                const uint8_t source_pixel = source_row[column];
                if(!transparent || source_pixel != 0)
                    destination_pixels[column] = palette[source_pixel];
            }
        }
        else if(destination_bits == 16)
        {
            auto *destination_pixels = reinterpret_cast<uint16_t *>(destination_row);
            for(int32_t column = 0; column < region.width; ++column)
            {
                uint8_t source_pixel = source_row[column];
                if(!transparent || source_pixel != 0)
                    destination_pixels[column] = static_cast<uint16_t>(palette[source_pixel]);
            }
        }
        else
        {
            for(int32_t column = 0; column < region.width; ++column)
            {
                uint8_t source_pixel = source_row[column];
                if(!transparent || source_pixel != 0)
                    destination_row[column] = convert_palette ? static_cast<uint8_t>(palette[source_pixel]) : source_pixel;
            }
        }
        source_row += source->sync_secondary_position;
        destination_row += destination->sync_secondary_position;
    }
}

void composite_scene_pixels(DisplaySceneNode *destination, int32_t destination_x, int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle, void *source_state, uint32_t mode,
    bool transparent, bool convert_palette, uint32_t destination_bits)
{
    CompositeRegion region{};
    if(prepare_composite_region(destination, destination_x, destination_y, source, rectangle, mode, region))
        composite_indexed_pixels(destination, source, source_state, region, transparent, convert_palette, destination_bits);
}

void composite_xrgb_pixels(DisplaySceneNode *destination, int32_t destination_x, int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle, uint32_t mode, bool transparent)
{
    CompositeRegion region{};
    if(!prepare_composite_region(destination, destination_x, destination_y, source, rectangle, mode, region))
        return;
    auto *source_row = reinterpret_cast<const uint8_t *>(static_cast<uintptr_t>(source->callback_position)) + region.source_y * source->sync_secondary_position
                     + region.source_x * static_cast<int32_t>(sizeof(uint32_t));
    auto *destination_row = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(destination->callback_position)) + region.destination_y * destination->sync_secondary_position
                          + region.destination_x * static_cast<int32_t>(sizeof(uint32_t));
    for(int32_t row_index = 0; row_index < region.height; ++row_index)
    {
        const auto *source_pixels = reinterpret_cast<const uint32_t *>(source_row);
        auto *destination_pixels = reinterpret_cast<uint32_t *>(destination_row);
        if(transparent)
        {
            for(int32_t column = 0; column < region.width; ++column)
                if((source_pixels[column] & 0xff000000) != 0)
                    destination_pixels[column] = source_pixels[column];
        }
        else
        {
            std::memcpy(destination_pixels, source_pixels, static_cast<size_t>(region.width) * sizeof(uint32_t));
        }
        source_row += source->sync_secondary_position;
        destination_row += destination->sync_secondary_position;
    }
}


void composite_transparent_8_to_8(DisplaySceneNode *destination, int32_t destination_x, int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle, void *source_state, uint32_t mode)
{
    composite_scene_pixels(destination, destination_x, destination_y, source, rectangle, source_state, mode, true, false, 8);
}

void composite_opaque_8_to_8(DisplaySceneNode *destination, int32_t destination_x, int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle, void *source_state, uint32_t mode)
{
    composite_scene_pixels(destination, destination_x, destination_y, source, rectangle, source_state, mode, false, false, 8);
}

void composite_transparent_indexed_to_8(DisplaySceneNode *destination, int32_t destination_x, int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle, void *source_state,
    uint32_t mode)
{
    composite_scene_pixels(destination, destination_x, destination_y, source, rectangle, source_state, mode, true, true, 8);
}

void composite_opaque_indexed_to_8(DisplaySceneNode *destination, int32_t destination_x, int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle, void *source_state,
    uint32_t mode)
{
    composite_scene_pixels(destination, destination_x, destination_y, source, rectangle, source_state, mode, false, true, 8);
}

void composite_transparent_indexed_to_16(DisplaySceneNode *destination, int32_t destination_x, int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle, void *source_state,
    uint32_t mode)
{
    composite_scene_pixels(destination, destination_x, destination_y, source, rectangle, source_state, mode, true, true, 16);
}

void composite_opaque_indexed_to_16(DisplaySceneNode *destination, int32_t destination_x, int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle, void *source_state,
    uint32_t mode)
{
    composite_scene_pixels(destination, destination_x, destination_y, source, rectangle, source_state, mode, false, true, 16);
}

void composite_transparent_indexed_to_32(DisplaySceneNode *destination, int32_t destination_x, int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle, void *source_state,
    uint32_t mode)
{
    composite_scene_pixels(destination, destination_x, destination_y, source, rectangle, source_state, mode, true, true, 32);
}

void composite_opaque_indexed_to_32(DisplaySceneNode *destination, int32_t destination_x, int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle, void *source_state,
    uint32_t mode)
{
    composite_scene_pixels(destination, destination_x, destination_y, source, rectangle, source_state, mode, false, true, 32);
}

void composite_transparent_32_to_32(DisplaySceneNode *destination, int32_t destination_x, int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle, void *, uint32_t mode)
{
    composite_xrgb_pixels(destination, destination_x, destination_y, source, rectangle, mode, true);
}

void composite_opaque_32_to_32(DisplaySceneNode *destination, int32_t destination_x, int32_t destination_y, DisplaySceneNode *source, DisplayRectangle *rectangle, void *, uint32_t mode)
{
    composite_xrgb_pixels(destination, destination_x, destination_y, source, rectangle, mode, false);
}

void build_indexed_to_16_palette(DisplayPixelFormatDescriptor *source_state, const DisplayPixelFormatDescriptor *destination_state)
{
    if(source_state->bits_per_pixel != 8 || source_state->palette_source == nullptr || destination_state->bits_per_pixel != 0x10)
        return;
    const auto count_bits = [](uint32_t mask, uint32_t &shift)
    {
        shift = 0;
        if(mask != 0)
            while(((mask >> shift) & 1) == 0)
                ++shift;
        uint32_t high_bit = 31;
        if(mask != 0)
            while((mask >> high_bit) == 0)
                --high_bit;
        return (high_bit + 1) - shift;
    };
    uint32_t red_shift;
    uint32_t green_shift;
    uint32_t blue_shift;
    uint32_t red_bits = count_bits(destination_state->red_mask, red_shift);
    uint32_t green_bits = count_bits(destination_state->green_mask, green_shift);
    uint32_t blue_bits = count_bits(destination_state->blue_mask, blue_shift);
    const uint32_t *source_palette = source_state->palette_source;
    auto *destination_palette = const_cast<uint32_t *>(source_state->palette_entries);
    for(uint32_t index = 0; index < source_state->palette_count; ++index)
    {
        uint32_t color = source_palette[index];
        auto rounded_component = [](uint8_t component, uint32_t bits)
        {
            uint8_t rounding = static_cast<uint8_t>(1u << (((8u - bits) >> 1) & 31));
            uint16_t rounded = static_cast<uint16_t>(component) + rounding;
            return static_cast<uint8_t>(rounded > 0xff ? 0xff : rounded);
        };
        uint8_t red = rounded_component(static_cast<uint8_t>(color), red_bits);
        uint8_t green = rounded_component(static_cast<uint8_t>(color >> 8), green_bits);
        uint8_t blue = rounded_component(static_cast<uint8_t>(color >> 16), blue_bits);
        destination_palette[index] = (static_cast<uint32_t>(red >> ((8u - red_bits) & 31)) << (red_shift & 31)) | (static_cast<uint32_t>(green >> ((8u - green_bits) & 31)) << (green_shift & 31))
                                   | (static_cast<uint32_t>(blue >> ((8u - blue_bits) & 31)) << (blue_shift & 31));
    }
}

void build_indexed_to_32_palette(DisplayPixelFormatDescriptor *source_state)
{
    if(source_state->bits_per_pixel != 8 || source_state->palette_source == nullptr || source_state->palette_entries == nullptr)
        return;
    const uint32_t *source_palette = source_state->palette_source;
    auto *destination_palette = const_cast<uint32_t *>(source_state->palette_entries);
    for(uint32_t index = 0; index < source_state->palette_count; ++index)
    {
        const uint32_t color = source_palette[index];
        const uint32_t red = color & 0xff;
        const uint32_t green = color >> 8 & 0xff;
        const uint32_t blue = color >> 16 & 0xff;
        destination_palette[index] = (index == 0 ? 0 : 0xff000000) | red << 16 | green << 8 | blue;
    }
}

void build_indexed_to_indexed_palette(DisplayPixelFormatDescriptor *source_state, const DisplayPixelFormatDescriptor *destination_state)
{
    if(source_state->bits_per_pixel != 8 || source_state->palette_source == nullptr || destination_state->bits_per_pixel != 8 || destination_state->palette_source == nullptr)
        return;
    uint32_t source_count = source_state->palette_count;
    uint32_t destination_count = destination_state->palette_count;
    const uint32_t *source_palette = source_state->palette_source;
    const uint32_t *destination_palette = destination_state->palette_source;
    auto *mapping = const_cast<uint32_t *>(source_state->palette_entries);
    for(uint32_t source_index = 0; source_index < source_count; ++source_index)
    {
        uint32_t threshold = 0;
        uint32_t destination_index;
        while(true)
        {
            for(destination_index = 0; destination_index < destination_count; ++destination_index)
            {
                uint32_t source_color = source_palette[source_index];
                uint32_t destination_color = destination_palette[destination_index];
                uint32_t maximum_difference = 0;
                for(uint32_t component = 0; component < 3; ++component)
                {
                    uint32_t source_component = (source_color >> (component * 8)) & 0xff;
                    uint32_t destination_component = (destination_color >> (component * 8)) & 0xff;
                    uint32_t difference = source_component < destination_component ? destination_component - source_component : source_component - destination_component;
                    if(maximum_difference < difference)
                        maximum_difference = difference;
                }
                if(maximum_difference <= threshold)
                    break;
            }
            if(destination_index < destination_count)
                break;
            threshold += 10;
            if(0x104 <= threshold)
                break;
        }
        mapping[source_index] = destination_index;
    }
}

bool configure_display_scene_palette(DisplaySceneNode *node, const uint32_t *palette, uint32_t count)
{
    if(node == nullptr)
        node = display_scene_root;
    bool result = false;
    if(begin_display_scene_update(reinterpret_cast<intptr_t>(node)) == 0)
    {
        if(node->rectangle_callback_format.bits_per_pixel == 8)
        {
            result = true;
            if(palette == nullptr)
            {
                node->rectangle_callback_format.palette_count = 0;
                node->rectangle_callback_format.palette_source = nullptr;
                node->rectangle_callback_format.palette_entries = nullptr;
                if(node == display_scene_root)
                {
                    for(DisplaySceneNode *entry = display_scene_head; entry != nullptr; entry = entry->next)
                        if(entry != node && entry->rectangle_callback_format.bits_per_pixel == 8 && entry->rectangle_callback_format.palette_source != nullptr)
                            entry->rectangle_callback = (entry->flags & DISPLAY_SCENE_OPAQUE) == 0 ? composite_transparent_8_to_8 : composite_opaque_8_to_8;
                    display_palette_source_state = nullptr;
                    display_lock_flags |= DISPLAY_SCENE_PALETTE_CHANGED;
                }
                else if(display_scene_root->rectangle_callback_format.bits_per_pixel == 8)
                {
                    node->rectangle_callback = (node->flags & DISPLAY_SCENE_OPAQUE) == 0 ? composite_transparent_8_to_8 : composite_opaque_8_to_8;
                }
                else
                {
                    node->rectangle_callback = nullptr;
                }
            }
            else
            {
                uint32_t copy_count = count & 0x3fffffff;
                std::memcpy(node->palette_source, palette, copy_count * sizeof(uint32_t));
                node->rectangle_callback_format.palette_count = count;
                node->rectangle_callback_format.palette_source = node->palette_source;
                node->rectangle_callback_format.palette_entries = node->palette_mapping;
                auto *node_state = &node->rectangle_callback_format;
                if(node == display_scene_root)
                {
                    for(DisplaySceneNode *entry = display_scene_head; entry != nullptr; entry = entry->next)
                    {
                        if(entry != node && entry->rectangle_callback_format.bits_per_pixel == 8 && entry->rectangle_callback_format.palette_source != nullptr)
                        {
                            build_indexed_to_indexed_palette(&entry->rectangle_callback_format, node_state);
                            entry->rectangle_callback = (entry->flags & DISPLAY_SCENE_OPAQUE) == 0 ? composite_transparent_indexed_to_8 : composite_opaque_indexed_to_8;
                        }
                    }
                    display_palette_source_state = node_state;
                    display_lock_flags |= DISPLAY_SCENE_PALETTE_CHANGED;
                }
                else if(display_scene_root->rectangle_callback_format.bits_per_pixel == 8)
                {
                    if(display_scene_root->rectangle_callback_format.palette_source != nullptr)
                    {
                        build_indexed_to_indexed_palette(node_state, &display_scene_root->rectangle_callback_format);
                        node->rectangle_callback = (node->flags & DISPLAY_SCENE_OPAQUE) == 0 ? composite_transparent_indexed_to_8 : composite_opaque_indexed_to_8;
                    }
                }
                else if(display_scene_root->rectangle_callback_format.bits_per_pixel == 0x10)
                {
                    build_indexed_to_16_palette(node_state, &display_scene_root->rectangle_callback_format);
                    node->rectangle_callback = (node->flags & DISPLAY_SCENE_OPAQUE) == 0 ? composite_transparent_indexed_to_16 : composite_opaque_indexed_to_16;
                }
                else if(display_scene_root->rectangle_callback_format.bits_per_pixel == 32)
                {
                    build_indexed_to_32_palette(node_state);
                    node->rectangle_callback = (node->flags & DISPLAY_SCENE_OPAQUE) == 0 ? composite_transparent_indexed_to_32 : composite_opaque_indexed_to_32;
                }
            }
        }
        end_display_scene_update(reinterpret_cast<intptr_t>(node), nullptr, nullptr);
    }
    return result;
}

void configure_display_scene_format(DisplaySceneNode *node, const DisplayPixelFormatDescriptor *format)
{
    node->rectangle_callback = nullptr;
    node->root_rectangle_callback = nullptr;
    node->rectangle_callback_format = *format;
    node->rectangle_callback_format.palette_count = 0;
    node->rectangle_callback_format.palette_source = nullptr;
    node->rectangle_callback_format.palette_entries = nullptr;
    if(display_scene_root == nullptr)
        return;
    uint32_t destination_bits = display_scene_root->rectangle_callback_format.bits_per_pixel;
    uint32_t source_bits = format->bits_per_pixel;
    if(node == display_scene_root)
    {
        if(source_bits == 8)
        {
            if(format->palette_source != nullptr)
                configure_display_scene_palette(node, format->palette_source, format->palette_count);
            node->root_rectangle_callback = fill_display_scene_rectangle_8;
        }
        else if(source_bits == 0x10)
        {
            node->root_rectangle_callback = fill_display_scene_rectangle_16;
        }
        else if(source_bits == 32)
        {
            node->root_rectangle_callback = fill_display_scene_rectangle_32;
        }
        return;
    }
    if(destination_bits == 8)
    {
        if(source_bits == 8)
        {
            if(format->palette_source != nullptr)
            {
                configure_display_scene_palette(node, format->palette_source, format->palette_count);
            }
            else if(format->palette_entries != nullptr)
            {
                std::memcpy(node->palette_mapping, format->palette_entries, sizeof(node->palette_mapping));
                node->rectangle_callback_format.palette_entries = node->palette_mapping;
                node->rectangle_callback = (node->flags & DISPLAY_SCENE_OPAQUE) == 0 ? composite_transparent_indexed_to_8 : composite_opaque_indexed_to_8;
            }
            else
            {
                node->rectangle_callback = (node->flags & DISPLAY_SCENE_OPAQUE) == 0 ? composite_transparent_8_to_8 : composite_opaque_8_to_8;
            }
            node->root_rectangle_callback = fill_display_scene_rectangle_8;
        }
        else if(source_bits == 0x10)
        {
            node->root_rectangle_callback = fill_display_scene_rectangle_16;
        }
    }
    else if(destination_bits == 0x10)
    {
        if(source_bits == 8)
        {
            if(format->palette_source != nullptr)
            {
                configure_display_scene_palette(node, format->palette_source, format->palette_count);
            }
            else if(format->palette_entries != nullptr)
            {
                std::memcpy(node->palette_mapping, format->palette_entries, sizeof(node->palette_mapping));
                node->rectangle_callback_format.palette_entries = node->palette_mapping;
                node->rectangle_callback = (node->flags & DISPLAY_SCENE_OPAQUE) == 0 ? composite_transparent_indexed_to_16 : composite_opaque_indexed_to_16;
            }
            node->root_rectangle_callback = fill_display_scene_rectangle_8;
        }
        else if(source_bits == 0x10)
        {
            node->root_rectangle_callback = fill_display_scene_rectangle_16;
        }
    }
    else if(destination_bits == 0x18)
    {
        if(source_bits == 8)
            node->root_rectangle_callback = fill_display_scene_rectangle_8;
        else if(source_bits == 0x10)
            node->root_rectangle_callback = fill_display_scene_rectangle_16;
    }
    else if(destination_bits == 32)
    {
        if(source_bits == 8)
        {
            if(format->palette_source != nullptr)
            {
                configure_display_scene_palette(node, format->palette_source, format->palette_count);
            }
            else if(format->palette_entries != nullptr)
            {
                std::memcpy(node->palette_mapping, format->palette_entries, sizeof(node->palette_mapping));
                node->rectangle_callback_format.palette_entries = node->palette_mapping;
                node->rectangle_callback = (node->flags & DISPLAY_SCENE_OPAQUE) == 0 ? composite_transparent_indexed_to_32 : composite_opaque_indexed_to_32;
            }
            node->root_rectangle_callback = fill_display_scene_rectangle_8;
        }
        else if(source_bits == 32)
        {
            node->rectangle_callback = (node->flags & DISPLAY_SCENE_OPAQUE) == 0 ? composite_transparent_32_to_32 : composite_opaque_32_to_32;
            node->root_rectangle_callback = fill_display_scene_rectangle_32;
        }
    }
}

DisplaySceneNode *acquire_display_scene_node(uint32_t index, int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t flags, intptr_t owner, DisplaySceneDescriptor *descriptor,
    const DisplayPixelFormatDescriptor *format)
{
    if(owner != 0 && descriptor == nullptr)
        return nullptr;
    if(format == nullptr)
        format = &default_display_pixel_format;
    if((display_lock_flags & DISPLAY_SCENE_HOST_INITIALIZED) == 0)
        return nullptr;
    DisplaySceneNode *result = nullptr;
    DisplaySceneNode *previous = nullptr;
    uint32_t previous_mode = 0;
    bool locked_node = false;
    int32_t requested_x = x;
    uint32_t requested_index = index;
    const DisplaySceneStorage requested_storage = format->bits_per_pixel == 8 ? DisplaySceneStorage::INDEXED_SOURCE : DisplaySceneStorage::XRGB_COMPOSITION;
    if((flags & 1) != 0)
    {
        requested_index = DISPLAY_SCENE_ROOT_INDEX;
        flags |= DISPLAY_SCENE_OPAQUE | DISPLAY_SCENE_STATIC;
    }
    while(true)
    {
        if(locked_node)
        {
            runtime_sleep(5);
            locked_node = false;
        }
        if(previous_mode == DISPLAY_SCENE_LOCK_MODE_MASK)
            wait_runtime_event(display_lock_release_event);
        if(previous_mode == DISPLAY_SCENE_LOCK_ACQUIRED)
            runtime_sleep(5);
        lock_runtime_mutex(display_lock_mutex);
        previous_mode = display_lock_flags & DISPLAY_SCENE_LOCK_MODE_MASK;
        if(previous_mode == 0)
        {
            previous_mode = DISPLAY_OPERATION_FAILED;
            DisplaySceneNode *existing = display_scene_head;
            previous = nullptr;
            bool matching_scene = false;
            while(existing != nullptr)
            {
                if(existing->scene_index < requested_index)
                {
                    previous = existing;
                    existing = existing->next;
                    continue;
                }
                if(existing->scene_index > requested_index)
                    break;
                if(existing->storage == requested_storage)
                {
                    if(requested_storage == DisplaySceneStorage::XRGB_COMPOSITION)
                    {
                        matching_scene = true;
                        break;
                    }
                    for(uint32_t owner_index = 0; owner_index < existing->owner_count; ++owner_index)
                    {
                        if(existing->owners[owner_index] == owner)
                        {
                            matching_scene = true;
                            break;
                        }
                    }
                    if(matching_scene)
                        break;
                }
                if(requested_storage == DisplaySceneStorage::XRGB_COMPOSITION)
                    break;
                previous = existing;
                existing = existing->next;
            }
            if(!matching_scene)
            {
                RuntimeHeap *heap = runtime_process_heap();
                auto *node = static_cast<DisplaySceneNode *>(allocate_runtime_heap(heap, 8, sizeof(DisplaySceneNode)));
                if(node != nullptr)
                {
                    uint32_t bytes_per_pixel = format->bits_per_pixel >> 3;
                    uint32_t pixel_bytes;
                    if(requested_index == DISPLAY_SCENE_ROOT_INDEX)
                    {
                        pixel_bytes = bytes_per_pixel * static_cast<uint32_t>(display_height) * static_cast<uint32_t>(display_width);
                        display_scene_root = node;
                        node->callback_first_position = display_scene_root_primary_position;
                        node->sync_secondary_position = static_cast<int32_t>(bytes_per_pixel * display_width);
                        node->width = display_width;
                        node->height = display_height;
                    }
                    else
                    {
                        pixel_bytes = bytes_per_pixel * height * width;
                        void *pixels = allocate_runtime_heap(heap, 0, pixel_bytes);
                        node->callback_first_position = reinterpret_cast<intptr_t>(pixels);
                        if(pixels != nullptr)
                        {
                            node->sync_secondary_position = static_cast<int32_t>(bytes_per_pixel * width);
                            node->width = static_cast<int32_t>(width);
                            node->height = static_cast<int32_t>(height);
                            node->x = requested_x;
                            node->y = y;
                            node->previous_x = requested_x;
                            node->previous_y = y;
                        }
                    }
                    if(node->callback_first_position != 0 && requested_storage == DisplaySceneStorage::XRGB_COMPOSITION)
                    {
                        const size_t indexed_bytes = static_cast<size_t>(node->width) * static_cast<size_t>(node->height);
                        node->indexed_backing = reinterpret_cast<intptr_t>(allocate_runtime_heap(heap, runtime_heap_zero_memory, indexed_bytes));
                    }
                    if((node->callback_first_position == 0 && requested_index != DISPLAY_SCENE_ROOT_INDEX)
                        || (requested_storage == DisplaySceneStorage::XRGB_COMPOSITION && node->indexed_backing == 0))
                    {
                        if(node->callback_first_position != 0 && requested_index != DISPLAY_SCENE_ROOT_INDEX)
                            free_runtime_heap(heap, 0, reinterpret_cast<void *>(static_cast<uintptr_t>(node->callback_first_position)));
                        free_runtime_heap(heap, 0, node);
                    }
                    else
                    {
                        if(node->callback_first_position != 0)
                        {
                            if((flags & DISPLAY_SCENE_PRIMARY) != 0)
                                std::memset(reinterpret_cast<void *>(static_cast<uintptr_t>(node->callback_first_position)), 0, pixel_bytes);
                            if(requested_index == DISPLAY_SCENE_ROOT_INDEX)
                                display_scene_root_secondary_position = node->sync_secondary_position;
                        }
                        node->callback_position = node->callback_first_position;
                        node->storage = requested_storage;
                        node->identifier = reinterpret_cast<intptr_t>(node);
                        node->surface = &display_scene_surface_state;
                        node->flags = (flags & ~(DISPLAY_SCENE_XRGB_COMPOSITION | DISPLAY_SCENE_PRIMARY_OWNER | DISPLAY_SCENE_PRIMARY | DISPLAY_SCENE_INDEXED)) | DISPLAY_SCENE_UPDATE_PENDING;
                        node->scene_index = requested_index;
                        node->accumulated_rectangle.left = node->width;
                        node->accumulated_rectangle.top = node->height;
                        if(owner != 0)
                        {
                            if((flags & DISPLAY_SCENE_PRIMARY_OWNER) != 0 && node->primary_owner == 0)
                                node->primary_owner = owner;
                            node->owners[0] = owner;
                            ++node->owner_count;
                            node->flags |= flags & ~(DISPLAY_SCENE_PRIMARY_OWNER | DISPLAY_SCENE_PRIMARY | DISPLAY_SCENE_PRESERVE_POSITION | DISPLAY_SCENE_PRESERVE_DIMENSIONS);
                            descriptor->x = 0;
                            descriptor->y = 0;
                            descriptor->width = static_cast<int16_t>(width);
                            descriptor->height = static_cast<int16_t>(height);
                            descriptor->pixels = node->callback_first_position;
                            descriptor->bits_per_pixel = format->bits_per_pixel;
                            descriptor->stride = width;
                            descriptor->indexed_pixels = node->indexed_backing;
                            descriptor->indexed_stride = static_cast<uint32_t>(node->width);
                        }
                        ++display_scene_count;
                        if(previous == nullptr)
                        {
                            node->next = display_scene_head;
                            display_scene_head = node;
                        }
                        else
                        {
                            node->next = previous->next;
                            previous->next = node;
                        }
                        configure_display_scene_format(node, format);
                        result = node;
                    }
                }
            }
            else
            {
                if((flags & DISPLAY_SCENE_XRGB_COMPOSITION) != 0)
                {
                    if(requested_x < existing->x)
                        requested_x = existing->x;
                    if(y < existing->y)
                        y = existing->y;
                    if(static_cast<uint32_t>(existing->x + existing->width) < static_cast<uint32_t>(requested_x) + width)
                        requested_x = (existing->width - static_cast<int32_t>(width)) + existing->x;
                    if(static_cast<uint32_t>(existing->y + existing->height) < static_cast<uint32_t>(y) + height)
                        y = (existing->height - static_cast<int32_t>(height)) + existing->y;
                }
                int32_t offset_x = requested_x - existing->x;
                int32_t offset_y = y - existing->y;
                bool owner_exists = false;
                for(uint32_t owner_index = 0; owner_index < existing->owner_count; ++owner_index)
                {
                    if(existing->owners[owner_index] == owner)
                    {
                        owner_exists = true;
                        break;
                    }
                }
                bool fits = offset_x >= 0 && offset_y >= 0 && static_cast<uint32_t>(offset_x) + width <= static_cast<uint32_t>(existing->width)
                         && static_cast<uint32_t>(offset_y) + height <= static_cast<uint32_t>(existing->height);
                uint32_t existing_flags = existing->flags;
                if((existing_flags & DISPLAY_SCENE_INDEXED) != 0 || owner_exists || !fits || (existing->owner_count == 0 && (existing_flags & DISPLAY_SCENE_STATIC) == 0))
                {
                    locked_node = true;
                    if(existing->lock_count == 0)
                    {
                        locked_node = false;
                        if((flags & DISPLAY_SCENE_PRESERVE_DIMENSIONS) != 0)
                        {
                            if(width <= static_cast<uint32_t>(existing->width))
                                width = existing->width;
                            if(height <= static_cast<uint32_t>(existing->height))
                                height = existing->height;
                        }
                        if((flags & DISPLAY_SCENE_PRESERVE_POSITION) != 0)
                        {
                            requested_x = existing->x;
                            y = existing->y;
                        }
                        if(((existing->owner_count == 0 && owner != 0) || (existing->owner_count == 1 && owner_exists)) && (existing_flags & (DISPLAY_SCENE_INDEXED | DISPLAY_SCENE_STATIC)) == 0)
                        {
                            uint32_t pixel_bytes = (format->bits_per_pixel >> 3) * height * width;
                            if(existing->rectangle_callback_format.bits_per_pixel == format->bits_per_pixel
                                && ((existing_flags & DISPLAY_SCENE_FIXED_SIZE) == 0 || (width == static_cast<uint32_t>(existing->width) && height == static_cast<uint32_t>(existing->height)))
                                && ((existing_flags & DISPLAY_SCENE_FIXED_POSITION) == 0 || (requested_x == existing->x && y == existing->y)))
                            {
                                DisplayRectangle old_rectangle{ existing->x, existing->y, existing->x + existing->width, existing->y + existing->height };
                                void *primary;
                                void *current;
                                void *alternate;
                                void *indexed_backing;
                                bool allocation_success = true;
                                const bool dimensions_changed = existing->width != static_cast<int32_t>(width) || existing->height != static_cast<int32_t>(height);
                                RuntimeHeap *heap = runtime_process_heap();
                                if(!dimensions_changed)
                                {
                                    primary = reinterpret_cast<void *>(static_cast<uintptr_t>(existing->callback_first_position));
                                    current = reinterpret_cast<void *>(static_cast<uintptr_t>(existing->callback_current_position));
                                    alternate = reinterpret_cast<void *>(static_cast<uintptr_t>(existing->callback_alternate_position));
                                    indexed_backing = reinterpret_cast<void *>(static_cast<uintptr_t>(existing->indexed_backing));
                                }
                                else
                                {
                                    bool had_current = existing->callback_current_position != 0;
                                    bool had_alternate = existing->callback_alternate_position != 0;
                                    current = nullptr;
                                    alternate = nullptr;
                                    indexed_backing = nullptr;
                                    primary = allocate_runtime_heap(heap, 0, pixel_bytes);
                                    allocation_success = primary != nullptr;
                                    if(had_current && allocation_success)
                                    {
                                        current = allocate_runtime_heap(heap, 0, pixel_bytes);
                                        if(current == nullptr)
                                            allocation_success = false;
                                    }
                                    if(had_alternate && allocation_success)
                                    {
                                        alternate = allocate_runtime_heap(heap, 0, pixel_bytes);
                                        if(alternate == nullptr)
                                            allocation_success = false;
                                    }
                                    if(existing->storage == DisplaySceneStorage::XRGB_COMPOSITION && allocation_success)
                                    {
                                        const size_t indexed_bytes = static_cast<size_t>(width) * static_cast<size_t>(height);
                                        indexed_backing = allocate_runtime_heap(heap, runtime_heap_zero_memory, indexed_bytes);
                                        if(indexed_backing == nullptr)
                                            allocation_success = false;
                                    }
                                }
                                if(allocation_success)
                                {
                                    if(dimensions_changed)
                                    {
                                        if(existing->callback_alternate_position != 0)
                                            free_runtime_heap(heap, 0, reinterpret_cast<void *>(static_cast<uintptr_t>(existing->callback_alternate_position)));
                                        if(existing->callback_current_position != 0)
                                            free_runtime_heap(heap, 0, reinterpret_cast<void *>(static_cast<uintptr_t>(existing->callback_current_position)));
                                        free_runtime_heap(heap, 0, reinterpret_cast<void *>(static_cast<uintptr_t>(existing->callback_first_position)));
                                        if(existing->indexed_backing != 0)
                                            free_runtime_heap(heap, 0, reinterpret_cast<void *>(static_cast<uintptr_t>(existing->indexed_backing)));
                                    }
                                    existing->callback_first_position = reinterpret_cast<intptr_t>(primary);
                                    existing->callback_current_position = reinterpret_cast<intptr_t>(current);
                                    existing->callback_alternate_position = reinterpret_cast<intptr_t>(alternate);
                                    existing->indexed_backing = reinterpret_cast<intptr_t>(indexed_backing);
                                    queue_display_rectangle(&old_rectangle);
                                    if((flags & DISPLAY_SCENE_PRIMARY_OWNER) != 0 && existing->primary_owner == 0)
                                        existing->primary_owner = owner;
                                    if((flags & DISPLAY_SCENE_PRIMARY) != 0)
                                    {
                                        std::memset(primary, 0, pixel_bytes);
                                        if(indexed_backing != nullptr)
                                            std::memset(indexed_backing, 0, static_cast<size_t>(width) * height);
                                    }
                                    existing->owners[0] = owner;
                                    existing->owner_count = 1;
                                    existing->reference_count = 0;
                                    existing->callback_position = reinterpret_cast<intptr_t>(primary);
                                    existing->sync_secondary_position = static_cast<int32_t>((format->bits_per_pixel >> 3) * width);
                                    existing->width = width;
                                    existing->height = height;
                                    existing->x = requested_x;
                                    existing->y = y;
                                    existing->previous_x = requested_x;
                                    existing->previous_y = y;
                                    existing->flags = flags | DISPLAY_SCENE_UPDATE_PENDING;
                                    descriptor->x = 0;
                                    descriptor->y = 0;
                                    descriptor->width = static_cast<int16_t>(width);
                                    descriptor->height = static_cast<int16_t>(height);
                                    descriptor->pixels = reinterpret_cast<intptr_t>(primary);
                                    descriptor->bits_per_pixel = format->bits_per_pixel;
                                    descriptor->stride = width;
                                    descriptor->indexed_pixels = existing->indexed_backing;
                                    descriptor->indexed_stride = static_cast<uint32_t>(existing->width);
                                    result = existing;
                                }
                                else
                                {
                                    if(alternate != nullptr)
                                        free_runtime_heap(heap, 0, alternate);
                                    if(current != nullptr)
                                        free_runtime_heap(heap, 0, current);
                                    if(primary != nullptr)
                                        free_runtime_heap(heap, 0, primary);
                                    if(indexed_backing != nullptr)
                                        free_runtime_heap(heap, 0, indexed_backing);
                                }
                            }
                        }
                        else if(owner == 0)
                        {
                            result = existing;
                        }
                    }
                }
                else if(existing->rectangle_callback_format.bits_per_pixel == format->bits_per_pixel)
                {
                    result = existing;
                    if(owner != 0)
                    {
                        if((flags & DISPLAY_SCENE_PRIMARY_OWNER) != 0 && existing->primary_owner == 0)
                            existing->primary_owner = owner;
                        if((flags & DISPLAY_SCENE_PRIMARY) != 0 && existing->owner_count == 0)
                        {
                            const uint32_t pixel_bytes = existing->rectangle_callback_format.bits_per_pixel >> 3;
                            std::memset(reinterpret_cast<void *>(static_cast<uintptr_t>(existing->callback_first_position)), 0,
                                static_cast<uint32_t>(existing->width) * static_cast<uint32_t>(existing->height) * pixel_bytes);
                            if(existing->indexed_backing != 0)
                            {
                                std::memset(reinterpret_cast<void *>(static_cast<uintptr_t>(existing->indexed_backing)), 0,
                                    static_cast<size_t>(existing->width) * static_cast<size_t>(existing->height));
                            }
                        }
                        existing->owners[existing->owner_count] = owner;
                        descriptor->x = static_cast<int16_t>(offset_x);
                        descriptor->y = static_cast<int16_t>(offset_y);
                        descriptor->width = static_cast<int16_t>(existing->width);
                        descriptor->height = static_cast<int16_t>(existing->height);
                        descriptor->pixels = existing->callback_first_position;
                        descriptor->bits_per_pixel = existing->rectangle_callback_format.bits_per_pixel;
                        descriptor->stride = static_cast<uint32_t>(existing->sync_secondary_position) / (existing->rectangle_callback_format.bits_per_pixel >> 3);
                        descriptor->indexed_pixels = existing->indexed_backing;
                        descriptor->indexed_stride = static_cast<uint32_t>(existing->width);
                        ++existing->owner_count;
                    }
                }
            }
            if(result != nullptr && (result->scene_index != DISPLAY_SCENE_ROOT_INDEX || result->reference_count == 0))
                ++result->reference_count;
        }
        unlock_runtime_mutex(display_lock_mutex);
        if((previous_mode & DISPLAY_SCENE_LOCK_MODE_MASK) != 0)
            continue;
        if(!locked_node)
            return result;
    }
}

uint32_t *initialize_display_scene_host(intptr_t primary_position, const DisplayPixelFormatDescriptor *format, int32_t width, int32_t height,
    int (*synchronize)(void *context, void *payload, uint32_t mode), void *context, uint32_t worker_interval)
{
    if(primary_position == 0)
        return nullptr;
    if((display_lock_flags & DISPLAY_SCENE_HOST_INITIALIZED) != 0)
        return &display_lock_flags;

    display_lock_flags = 0;
    display_lock_mutex = nullptr;
    display_lock_owner_thread = {};
    display_lock_recursion_count = 0;
    display_lock_release_event = nullptr;
    display_clip_bounds = {};
    display_palette_source_state = nullptr;
    display_scene_worker_thread = nullptr;
    display_lock_gate_event = nullptr;
    display_lock_busy = 0;
    display_pending_rectangle = {};
    display_width = 0;
    display_height = 0;
    display_scene_synchronize = nullptr;
    display_scene_sync_context = nullptr;
    display_scene_worker_interval = 0;
    display_scene_worker_rate = 0;
    display_scene_count = 0;
    display_scene_root = nullptr;
    display_scene_head = nullptr;

    display_lock_mutex = new (std::nothrow) RuntimeMutex;
    display_lock_release_event = new (std::nothrow) RuntimeManualResetEvent(true);
    if(display_lock_mutex == nullptr || display_lock_release_event == nullptr)
    {
        delete display_lock_release_event;
        delete display_lock_mutex;
        display_lock_release_event = nullptr;
        display_lock_mutex = nullptr;
        return nullptr;
    }
    display_lock_gate_event = new (std::nothrow) RuntimeManualResetEvent(true);
    if(display_lock_gate_event == nullptr)
    {
        delete display_lock_release_event;
        delete display_lock_mutex;
        display_lock_release_event = nullptr;
        display_lock_mutex = nullptr;
        return nullptr;
    }

    display_width = width;
    display_height = height;
    display_scene_surface_state.width = width;
    display_scene_surface_state.height = height;
    display_lock_flags = DISPLAY_SCENE_HOST_INITIALIZED;
    display_scene_root_primary_position = primary_position;
    display_scene_root = acquire_display_scene_node(0, 0, 0, 0, 0, 1, 0, nullptr, format);
    if(display_scene_root == nullptr)
    {
        delete display_lock_release_event;
        delete display_lock_gate_event;
        delete display_lock_mutex;
        display_lock_release_event = nullptr;
        display_lock_gate_event = nullptr;
        display_lock_mutex = nullptr;
        display_lock_flags = 0;
        return nullptr;
    }
    if(synchronize != nullptr)
    {
        display_scene_synchronize = synchronize;
        display_scene_sync_context = context;
        display_scene_worker_interval = worker_interval;
        display_scene_worker_thread = new (std::nothrow) std::jthread([] { run_display_scene_worker(&display_lock_flags); });
        if(display_scene_worker_thread == nullptr)
        {
            display_lock_flags |= DISPLAY_SCENE_HOST_SHUTDOWN_REQUESTED;
            release_display_scene_node(reinterpret_cast<intptr_t>(display_scene_root), 0);
            delete display_lock_release_event;
            delete display_lock_gate_event;
            delete display_lock_mutex;
            display_lock_release_event = nullptr;
            display_lock_gate_event = nullptr;
            display_lock_mutex = nullptr;
            display_lock_flags = 0;
            return nullptr;
        }
    }
    display_clip_bounds = { 0, 0, width, height };
    display_pending_rectangle.left = width;
    display_pending_rectangle.top = height;
    return &display_lock_flags;
}

uint32_t shutdown_display_scene_host()
{
    if((display_lock_flags & DISPLAY_SCENE_HOST_INITIALIZED) == 0)
        return DISPLAY_OPERATION_FAILED;
    lock_runtime_mutex(display_lock_mutex);
    if(display_scene_count != 1)
    {
        unlock_runtime_mutex(display_lock_mutex);
        return DISPLAY_OPERATION_FAILED;
    }
    display_lock_flags = (display_lock_flags & ~DISPLAY_SCENE_HOST_INITIALIZED) | DISPLAY_SCENE_HOST_SHUTDOWN_REQUESTED;
    unlock_runtime_mutex(display_lock_mutex);
    if(display_scene_worker_thread != nullptr)
    {
        display_scene_worker_thread->join();
        delete display_scene_worker_thread;
        display_scene_worker_thread = nullptr;
    }
    release_display_scene_node(reinterpret_cast<intptr_t>(display_scene_root), 0);
    delete display_lock_release_event;
    delete display_lock_gate_event;
    delete display_lock_mutex;
    display_lock_release_event = nullptr;
    display_lock_gate_event = nullptr;
    display_lock_mutex = nullptr;
    return DISPLAY_OPERATION_SUCCESS;
}

void run_display_scene_worker(uint32_t *flags)
{
    uint32_t frame_start = runtime_milliseconds();
    uint32_t rate_start = frame_start;
    uint32_t frame_count = 0;
    while((*flags & DISPLAY_SCENE_HOST_SHUTDOWN_REQUESTED) == 0)
    {
        DisplayRectangle primary_rectangle{};
        DisplayRectangle secondary_rectangle{};
        uint32_t dirty_flags = 0;
        const uint32_t acquire_result = acquire_display_lock(&primary_rectangle, &secondary_rectangle, &dirty_flags);
        if(acquire_result == DISPLAY_OPERATION_SUCCESS)
        {
            const int synchronize_result = dirty_flags != 0 ? synchronize_display_scene_node(display_scene_root, &primary_rectangle) : 0;
            if(dirty_flags != 0 && synchronize_result != 0)
            {
                *flags |= DISPLAY_SCENE_WORKER_READY;
                if((dirty_flags & DISPLAY_DIRTY_SECONDARY) != 0 && display_scene_root->root_rectangle_callback != nullptr)
                    display_scene_root->root_rectangle_callback(display_scene_root, &secondary_rectangle, 0);
                if((dirty_flags & DISPLAY_DIRTY_PRIMARY) != 0)
                {
                    for(DisplaySceneNode *node = display_scene_head; node != nullptr; node = node->next)
                        if(node->rectangle_callback != nullptr)
                            node->rectangle_callback(display_scene_root, 0, 0, node, &primary_rectangle, &node->rectangle_callback_format, 0x01000000);
                }
                publish_display_scene_node(display_scene_root);
                release_pending_display_lock();
                if((*flags & DISPLAY_SCENE_PALETTE_CHANGED) != 0)
                {
                    display_scene_synchronize(display_scene_sync_context, display_palette_source_state, 2);
                    *flags &= ~DISPLAY_SCENE_PALETTE_CHANGED;
                }
                display_scene_synchronize(display_scene_sync_context, &primary_rectangle, 1);
            }
            release_display_lock();
        }
        uint32_t now = runtime_milliseconds();
        uint32_t elapsed = now - frame_start;
        if(elapsed < display_scene_worker_interval)
        {
            uint32_t delay = display_scene_worker_interval - elapsed;
            runtime_sleep(delay);
            frame_start = now + delay;
        }
        else
        {
            runtime_sleep(0);
            frame_start = now;
        }
        if(dirty_flags != 0)
            ++frame_count;
        if(now - rate_start >= 1000)
        {
            display_scene_worker_rate = frame_count;
            frame_count = 0;
            rate_start = now;
        }
    }
}

uint32_t release_display_scene_node(intptr_t identifier, intptr_t owner)
{
    if((identifier == 0 && owner == 0) || (display_lock_flags & DISPLAY_SCENE_HOST_INITIALIZED) == 0)
        return DISPLAY_OPERATION_FAILED;
    uint32_t previous_mode = 0;
    uint32_t previous_busy = 0;
    bool locked_node = false;
    while(true)
    {
        if(locked_node)
        {
            runtime_sleep(5);
            locked_node = false;
        }
        if(previous_busy != 0)
            wait_runtime_event(display_lock_gate_event);
        if(previous_mode == DISPLAY_SCENE_LOCK_MODE_MASK)
            wait_runtime_event(display_lock_release_event);
        if(previous_mode == DISPLAY_SCENE_LOCK_ACQUIRED)
            runtime_sleep(5);
        lock_runtime_mutex(display_lock_mutex);
        uint32_t observed_busy = display_lock_busy;
        previous_mode = display_lock_flags & DISPLAY_SCENE_LOCK_MODE_MASK;
        previous_busy = display_lock_busy;
        uint32_t result = previous_mode;
        if(previous_mode == 0 && display_lock_busy == 0)
        {
            result = DISPLAY_OPERATION_FAILED;
            DisplaySceneNode *previous = nullptr;
            DisplaySceneNode *node = display_scene_head;
            while(node != nullptr)
            {
                DisplaySceneNode *next = node->next;
                DisplaySceneNode *retained_node = node;
                if(node->identifier == identifier || identifier == 0)
                {
                    if(node->lock_count == 0)
                    {
                        intptr_t remaining_owner = owner;
                        if(owner != 0)
                        {
                            for(uint32_t owner_index = 0; owner_index < node->owner_count; ++owner_index)
                            {
                                if(node->owners[owner_index] == owner)
                                {
                                    --node->owner_count;
                                    node->owners[owner_index] = node->owners[node->owner_count];
                                    node->owners[node->owner_count] = 0;
                                    if(node->owner_count == 0)
                                    {
                                        node->flags = (node->flags & ~DISPLAY_SCENE_INDEXED) | DISPLAY_SCENE_UPDATE_PENDING;
                                        if(node->rectangle_callback_format.bits_per_pixel == 8)
                                        {
                                            node->rectangle_callback_format.palette_count = 0;
                                            node->rectangle_callback_format.palette_source = nullptr;
                                            node->rectangle_callback_format.palette_entries = nullptr;
                                            if(node != display_scene_root)
                                            {
                                                if(display_scene_root->rectangle_callback_format.bits_per_pixel == 8)
                                                    node->rectangle_callback = (node->flags & DISPLAY_SCENE_OPAQUE) == 0 ? composite_transparent_8_to_8 : composite_opaque_8_to_8;
                                                else
                                                    node->rectangle_callback = nullptr;
                                            }
                                        }
                                    }
                                    if(node->primary_owner == owner)
                                        node->primary_owner = 0;
                                    result = DISPLAY_OPERATION_SUCCESS;
                                    remaining_owner = 0;
                                    break;
                                }
                            }
                        }
                        if(static_cast<int32_t>(node->scene_index) != DISPLAY_SCENE_ROOT_INDEX || (display_lock_flags & DISPLAY_SCENE_HOST_SHUTDOWN_REQUESTED) != 0)
                        {
                            if(remaining_owner == 0 && node->owner_count < node->reference_count)
                            {
                                result = DISPLAY_OPERATION_SUCCESS;
                                --node->reference_count;
                            }
                            if(node->reference_count == 0)
                            {
                                DisplayRectangle dirty_rectangle{};
                                dirty_rectangle.left = node->x <= node->previous_x ? node->x : node->previous_x;
                                dirty_rectangle.top = node->y <= node->previous_y ? node->y : node->previous_y;
                                dirty_rectangle.right = node->width + (node->x < node->previous_x ? node->previous_x : node->x);
                                dirty_rectangle.bottom = node->height + (node->y < node->previous_y ? node->previous_y : node->y);
                                queue_display_rectangle(&dirty_rectangle);
                                --display_scene_count;
                                if(previous == nullptr)
                                    display_scene_head = next;
                                else
                                    previous->next = next;
                                RuntimeHeap *heap = runtime_process_heap();
                                DisplaySceneCallbackNode *callback = node->callbacks;
                                while(callback != nullptr)
                                {
                                    DisplaySceneCallbackNode *next_callback = callback->next;
                                    free_runtime_heap(heap, 0, callback);
                                    callback = next_callback;
                                }
                                if(node->callback_alternate_position != 0)
                                    free_runtime_heap(heap, 0, reinterpret_cast<void *>(static_cast<uintptr_t>(node->callback_alternate_position)));
                                if(node->callback_current_position != 0)
                                    free_runtime_heap(heap, 0, reinterpret_cast<void *>(static_cast<uintptr_t>(node->callback_current_position)));
                                if(static_cast<int32_t>(node->scene_index) == DISPLAY_SCENE_ROOT_INDEX)
                                    display_scene_root = nullptr;
                                else
                                    free_runtime_heap(heap, 0, reinterpret_cast<void *>(static_cast<uintptr_t>(node->callback_first_position)));
                                if(node->indexed_backing != 0)
                                    free_runtime_heap(heap, 0, reinterpret_cast<void *>(static_cast<uintptr_t>(node->indexed_backing)));
                                free_runtime_heap(heap, 0, node);
                                retained_node = nullptr;
                            }
                        }
                    }
                    else
                    {
                        locked_node = true;
                    }
                    if(identifier != 0)
                        break;
                }
                node = next;
                if(retained_node != nullptr)
                    previous = retained_node;
            }
        }
        unlock_runtime_mutex(display_lock_mutex);
        if((previous_mode & DISPLAY_SCENE_LOCK_MODE_MASK) != 0 || observed_busy != 0)
            continue;
        if(!locked_node)
            return result;
    }
}

void fill_display_scene_rectangle_16(DisplaySceneNode *node, DisplayRectangle *rectangle, int value)
{
    if(rectangle == nullptr || node == nullptr)
        return;
    int32_t left = rectangle->left;
    int32_t top = rectangle->top;
    int32_t right = rectangle->right;
    int32_t bottom = rectangle->bottom;
    if(left < 0)
        left = 0;
    if(top < 0)
        top = 0;
    if(node->width < right)
        right = node->width;
    if(node->height < bottom)
        bottom = node->height;
    int32_t row_width = right - left;
    int32_t row_count = bottom - top;
    if(row_width != 0 && left <= right && row_count != 0 && top <= bottom)
    {
        auto *row = reinterpret_cast<uint16_t *>(
            reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(node->callback_first_position)) + top * node->sync_secondary_position + left * static_cast<int32_t>(sizeof(uint16_t)));
        do
        {
            for(int32_t column = 0; column < row_width; ++column)
                row[column] = static_cast<uint16_t>(value);
            row = reinterpret_cast<uint16_t *>(reinterpret_cast<uint8_t *>(row) + node->sync_secondary_position);
            --row_count;
        } while(row_count != 0);
    }
}

void fill_display_scene_rectangle_32(DisplaySceneNode *node, DisplayRectangle *rectangle, int value)
{
    if(rectangle == nullptr || node == nullptr)
        return;
    const int32_t left = std::clamp(rectangle->left, 0, node->width);
    const int32_t top = std::clamp(rectangle->top, 0, node->height);
    const int32_t right = std::clamp(rectangle->right, 0, node->width);
    const int32_t bottom = std::clamp(rectangle->bottom, 0, node->height);
    if(left >= right || top >= bottom)
        return;
    auto *row = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(node->callback_first_position)) + top * node->sync_secondary_position + left * static_cast<int32_t>(sizeof(uint32_t));
    for(int32_t y = top; y < bottom; ++y)
    {
        std::fill_n(reinterpret_cast<uint32_t *>(row), right - left, static_cast<uint32_t>(value));
        row += node->sync_secondary_position;
    }
    if(node->indexed_backing != 0)
    {
        auto *indexed_row = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(node->indexed_backing)) + static_cast<size_t>(top) * node->width + left;
        for(int32_t y = top; y < bottom; ++y)
        {
            std::fill_n(indexed_row, right - left, uint8_t{ 0 });
            indexed_row += node->width;
        }
    }
}

int synchronize_display_scene_node(DisplaySceneNode *node, DisplayRectangle *output_rectangle)
{
    DisplayRectangle geometry{ 0, 0, node->width, node->height };
    intptr_t primary_position = node->callback_first_position;
    int32_t secondary_position = node->sync_secondary_position;
    DisplaySyncRequest request{ node == display_scene_root ? nullptr : node, &geometry, &secondary_position, &primary_position };
    int synchronized = display_scene_synchronize(display_scene_sync_context, &request, 0x10000);
    if(synchronized == 0)
        return 0;
    synchronized = 1;
    if(node->sync_secondary_position != secondary_position || node->callback_first_position != primary_position || node->width != geometry.right || node->height != geometry.bottom)
    {
        if(node->width == geometry.right && node->height == geometry.bottom)
        {
            if(output_rectangle != nullptr)
                *output_rectangle = geometry;
            node->callback_first_position = primary_position;
            node->callback_position = primary_position;
            node->sync_secondary_position = secondary_position;
            if(node == display_scene_root)
            {
                display_scene_root_primary_position = primary_position;
                display_scene_root_secondary_position = secondary_position;
            }
        }
        else
        {
            synchronized = 0;
            node->width = geometry.right;
            node->height = geometry.bottom;
            if(node == display_scene_root)
            {
                display_width = geometry.right;
                display_height = geometry.bottom;
            }
            queue_display_rectangle(&geometry);
            display_scene_synchronize(display_scene_sync_context, &request, 0x20000);
        }
    }
    return synchronized;
}

void publish_display_scene_node(DisplaySceneNode *node)
{
    DisplayRectangle geometry{ 0, 0, node->width, node->height };
    intptr_t primary_position = node->callback_first_position;
    int32_t secondary_position = node->sync_secondary_position;
    DisplaySyncRequest request{ node == display_scene_root ? nullptr : node, &geometry, &secondary_position, &primary_position };
    display_scene_synchronize(display_scene_sync_context, &request, 0x20000);
}

uint32_t dispatch_display_scene_update(void *target, uint32_t options)
{
    if((display_lock_flags & DISPLAY_SCENE_HOST_INITIALIZED) == 0 || display_scene_synchronize == nullptr)
        return DISPLAY_OPERATION_FAILED;
    uint32_t result = DISPLAY_OPERATION_LOCK_NOT_OWNED;
    RuntimeThreadId thread_id = runtime_thread_id();
    if((display_lock_flags & DISPLAY_SCENE_LOCK_ACQUIRED) != 0 && display_lock_owner_thread == thread_id)
    {
        result = DISPLAY_OPERATION_FAILED;
        DisplayRectangle local_rectangle;
        DisplayRectangle *rectangle = static_cast<DisplayRectangle *>(target);
        if((options & 0x100) != 0)
        {
            local_rectangle = { display_width, display_height, 0, 0 };
            DisplaySceneNode *node = static_cast<DisplaySceneNode *>(target);
            rectangle = &local_rectangle;
            if(contains_display_scene_node(reinterpret_cast<intptr_t>(node)))
                accumulate_scene_node_rectangle(rectangle, node);
        }
        uint32_t attempts = 0;
        while(true)
        {
            if(synchronize_display_scene_node(display_scene_root, rectangle) == 0)
                ++attempts;
            else
                attempts = 0;
            if(attempts == 0)
                break;
            if(attempts >= 10)
                return DISPLAY_OPERATION_FAILED;
            runtime_sleep(5);
        }
        if(!constrain_display_rectangle_to_surface(rectangle))
        {
            publish_display_scene_node(display_scene_root);
        }
        else
        {
            DisplayRectangle secondary_rectangle = *rectangle;
            for(DisplaySceneNode *node = display_scene_head; node != nullptr; node = node->next)
                if((node->flags & DISPLAY_SCENE_OPAQUE) != 0)
                    trim_display_rectangle_overlap(&secondary_rectangle, node);
            bool secondary_valid = constrain_display_rectangle_to_surface(&secondary_rectangle);
            if(secondary_valid && display_scene_root->root_rectangle_callback != nullptr)
                display_scene_root->root_rectangle_callback(display_scene_root, &secondary_rectangle, 0);
            for(DisplaySceneNode *node = display_scene_head; node != nullptr; node = node->next)
                if(node->rectangle_callback != nullptr)
                    node->rectangle_callback(display_scene_root, 0, 0, node, rectangle, &node->rectangle_callback_format, 0x01000000);
            publish_display_scene_node(display_scene_root);
            if((options & 0x200) == 0)
                display_scene_synchronize(display_scene_sync_context, rectangle, 1);
            result = DISPLAY_OPERATION_SUCCESS;
        }
    }
    return result;
}

bool contains_display_scene_node(intptr_t identifier)
{
    if((display_lock_flags & DISPLAY_SCENE_HOST_INITIALIZED) == 0)
        return false;
    bool found = false;
    lock_runtime_mutex(display_lock_mutex);
    for(DisplaySceneNode *node = display_scene_head; node != nullptr; node = node->next)
    {
        if(node->identifier == identifier)
        {
            found = true;
            break;
        }
    }
    unlock_runtime_mutex(display_lock_mutex);
    return found;
}

} // namespace freegag
