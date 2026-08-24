#include "binary_stream.h"
#include <limits>

namespace gag
{
StandardBinaryInputStream::StandardBinaryInputStream(const char *path)
    : state_(std::make_shared<SharedBinaryInputState>(path))
{
}

bool StandardBinaryInputStream::is_open() const
{
    return state_->is_open();
}

bool StandardBinaryInputStream::seek(uint32_t offset)
{
    if(offset > state_->size())
    {
        return false;
    }
    offset_ = offset;
    return true;
}

uint32_t StandardBinaryInputStream::size()
{
    return state_->size();
}

uint32_t StandardBinaryInputStream::read(void *destination, uint32_t size)
{
    const uint32_t transferred = state_->read_at(offset_, destination, size);
    offset_ += transferred;
    return transferred;
}

std::shared_ptr<SharedBinaryInputState> StandardBinaryInputStream::shared_state() const
{
    return state_;
}

StandardBinaryOutputStream::StandardBinaryOutputStream(const char *path)
    : stream_(path, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc)
{
}

bool StandardBinaryOutputStream::is_open() const
{
    return stream_.is_open();
}

bool StandardBinaryOutputStream::seek(uint32_t offset)
{
    stream_.clear();
    stream_.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    stream_.seekp(static_cast<std::streamoff>(offset), std::ios::beg);
    return static_cast<bool>(stream_);
}

uint32_t StandardBinaryOutputStream::position()
{
    const std::streampos position = stream_.tellp();
    if(position == std::streampos(-1) || position > static_cast<std::streamoff>(UINT32_MAX))
    {
        return 0;
    }
    return static_cast<uint32_t>(position);
}

bool StandardBinaryOutputStream::write(const void *source, uint32_t size)
{
    stream_.write(static_cast<const char *>(source), static_cast<std::streamsize>(size));
    return static_cast<bool>(stream_);
}

bool StandardBinaryOutputStream::flush()
{
    stream_.flush();
    return static_cast<bool>(stream_);
}

SharedBinaryInputState::SharedBinaryInputState(const char *path)
    : stream_(path, std::ios::binary)
{
    if(!stream_)
    {
        return;
    }
    stream_.seekg(0, std::ios::end);
    const std::streampos end = stream_.tellg();
    if(end != std::streampos(-1) && end <= static_cast<std::streamoff>(UINT32_MAX))
    {
        size_ = static_cast<uint32_t>(end);
    }
    stream_.seekg(0, std::ios::beg);
}

bool SharedBinaryInputState::is_open() const
{
    return stream_.is_open();
}

uint32_t SharedBinaryInputState::size() const
{
    return size_;
}

uint32_t SharedBinaryInputState::read_at(uint32_t offset, void *destination, uint32_t size)
{
    std::lock_guard lock(mutex_);
    stream_.clear();
    stream_.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    if(!stream_)
    {
        return 0;
    }
    stream_.read(static_cast<char *>(destination), static_cast<std::streamsize>(size));
    return static_cast<uint32_t>(stream_.gcount());
}
} // namespace gag
