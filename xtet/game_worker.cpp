#include "game_worker.h"
#include <chrono>

namespace xtet
{

GameWorker::~GameWorker()
{
    stop();
}

bool GameWorker::start(const std::function<std::uint32_t()> &interval_callback, const std::function<void()> &tick_callback, const std::function<void()> &failure_callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if(thread_.joinable() || !interval_callback || !tick_callback)
        return false;
    interval_callback_ = interval_callback;
    tick_callback_ = tick_callback;
    failure_callback_ = failure_callback;
    enabled_ = false;
    stopping_ = false;
    try
    {
        thread_ = std::thread([this]() { run(); });
    }
    catch(...)
    {
        interval_callback_ = {};
        tick_callback_ = {};
        failure_callback_ = {};
        return false;
    }
    return true;
}

void GameWorker::setEnabled(bool enabled)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        enabled_ = enabled;
    }
    condition_.notify_all();
}

void GameWorker::stop()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
        enabled_ = false;
    }
    condition_.notify_all();
    if(thread_.joinable())
        thread_.join();
    std::lock_guard<std::mutex> lock(mutex_);
    interval_callback_ = {};
    tick_callback_ = {};
    failure_callback_ = {};
    stopping_ = false;
}

bool GameWorker::running() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return thread_.joinable() && !stopping_;
}

void GameWorker::run()
{
    try
    {
        while(true)
        {
            const std::uint32_t interval = interval_callback_();
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait_for(lock, std::chrono::milliseconds(interval), [this]() { return stopping_; });
            if(stopping_)
                return;
            const bool enabled = enabled_;
            lock.unlock();
            if(enabled)
                tick_callback_();
        }
    }
    catch(...)
    {
        if(failure_callback_)
            failure_callback_();
    }
}

} // namespace xtet
