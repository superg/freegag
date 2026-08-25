#include "shared_binary_file.h"
#include <limits>

namespace freegag
{
SharedBinaryFile::SharedBinaryFile(const char *path)
    : stream_(path, std::ios::binary)
{
    if(!stream_)
        return;
    stream_.seekg(0, std::ios::end);
    const std::streampos end = stream_.tellg();
    if(end != std::streampos(-1) && end <= static_cast<std::streamoff>(UINT32_MAX))
        size_ = static_cast<uint32_t>(end);
    stream_.seekg(0, std::ios::beg);
}

bool SharedBinaryFile::is_open() const
{
    return stream_.is_open();
}

uint32_t SharedBinaryFile::size() const
{
    return size_;
}

uint32_t SharedBinaryFile::read_at(uint32_t offset, void *destination, uint32_t size)
{
    std::lock_guard lock(mutex_);
    stream_.clear();
    stream_.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    if(!stream_)
        return 0;
    stream_.read(static_cast<char *>(destination), static_cast<std::streamsize>(size));
    return static_cast<uint32_t>(stream_.gcount());
}
} // namespace freegag
