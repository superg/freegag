#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <stdint.h>
#include <thread>

namespace xtet
{

class GameWorker
{
public:
    ~GameWorker();
    bool start(const std::function<uint32_t()> &interval_callback, const std::function<void()> &tick_callback, const std::function<void()> &failure_callback = {});
    void setEnabled(bool enabled);
    void stop();
    bool running() const;

private:
    void run();

    std::function<uint32_t()> interval_callback_;
    std::function<void()> tick_callback_;
    std::function<void()> failure_callback_;
    std::thread thread_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    bool enabled_{};
    bool stopping_{};
};

} // namespace xtet
