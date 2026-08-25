#pragma once

#include <cstdint>
#include <fstream>
#include <mutex>

namespace freegag
{
class SharedBinaryFile
{
public:
    explicit SharedBinaryFile(const char *path);
    bool is_open() const;
    uint32_t size() const;
    uint32_t read_at(uint32_t offset, void *destination, uint32_t size);

private:
    mutable std::mutex mutex_;
    std::ifstream stream_;
    uint32_t size_{};
};
} // namespace freegag
