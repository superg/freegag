#pragma once

#include <cstdint>
#include <fstream>
#include <memory>
#include <mutex>

namespace gag
{
class SharedBinaryInputState;

class BinaryInputStream
{
public:
    virtual ~BinaryInputStream() = default;
    virtual bool seek(uint32_t offset) = 0;
    virtual uint32_t size() = 0;
    virtual uint32_t read(void *destination, uint32_t size) = 0;
};

class BinaryOutputStream
{
public:
    virtual ~BinaryOutputStream() = default;
    virtual bool seek(uint32_t offset) = 0;
    virtual uint32_t position() = 0;
    virtual bool write(const void *source, uint32_t size) = 0;
    virtual bool flush() = 0;
};

class StandardBinaryInputStream final : public BinaryInputStream
{
public:
    explicit StandardBinaryInputStream(const char *path);
    bool is_open() const;
    bool seek(uint32_t offset) override;
    uint32_t size() override;
    uint32_t read(void *destination, uint32_t size) override;
    std::shared_ptr<SharedBinaryInputState> shared_state() const;

private:
    std::shared_ptr<SharedBinaryInputState> state_;
    uint32_t offset_{};
};

class StandardBinaryOutputStream final : public BinaryOutputStream
{
public:
    explicit StandardBinaryOutputStream(const char *path);
    bool is_open() const;
    bool seek(uint32_t offset) override;
    uint32_t position() override;
    bool write(const void *source, uint32_t size) override;
    bool flush() override;

private:
    std::fstream stream_;
};

class SharedBinaryInputState
{
public:
    explicit SharedBinaryInputState(const char *path);
    bool is_open() const;
    uint32_t size() const;
    uint32_t read_at(uint32_t offset, void *destination, uint32_t size);

private:
    mutable std::mutex mutex_;
    std::ifstream stream_;
    uint32_t size_{};
};
} // namespace gag
