#include <windows.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace
{

struct ResourceIdentity
{
    std::uint16_t type;
    std::uint16_t name;
    std::uint16_t language;
};

constexpr std::array<ResourceIdentity, 9> resources{
    {
     { 2, 112, 1049 },
     { 3, 1, 1049 },
     { 3, 2, 1049 },
     { 3, 3, 1033 },
     { 5, 101, 1033 },
     { 5, 103, 1033 },
     { 5, 110, 1049 },
     { 14, 105, 1033 },
     { 14, 109, 1049 },
     }
};

bool compare_resource(HMODULE original, HMODULE reconstructed, const ResourceIdentity &identity)
{
    HRSRC original_resource = FindResourceExA(original, MAKEINTRESOURCEA(identity.type), MAKEINTRESOURCEA(identity.name), identity.language);
    HRSRC reconstructed_resource = FindResourceExA(reconstructed, MAKEINTRESOURCEA(identity.type), MAKEINTRESOURCEA(identity.name), identity.language);
    if(original_resource == nullptr || reconstructed_resource == nullptr)
    {
        return false;
    }

    const DWORD original_size = SizeofResource(original, original_resource);
    const DWORD reconstructed_size = SizeofResource(reconstructed, reconstructed_resource);
    if(original_size != reconstructed_size)
    {
        return false;
    }

    HGLOBAL original_handle = LoadResource(original, original_resource);
    HGLOBAL reconstructed_handle = LoadResource(reconstructed, reconstructed_resource);
    const void *original_data = original_handle == nullptr ? nullptr : LockResource(original_handle);
    const void *reconstructed_data = reconstructed_handle == nullptr ? nullptr : LockResource(reconstructed_handle);
    return original_data != nullptr && reconstructed_data != nullptr && std::memcmp(original_data, reconstructed_data, original_size) == 0;
}

} // namespace

int main(int argc, char **argv)
{
    if(argc != 3)
    {
        return 2;
    }

    HMODULE original = LoadLibraryExA(argv[1], nullptr, LOAD_LIBRARY_AS_DATAFILE);
    HMODULE reconstructed = LoadLibraryExA(argv[2], nullptr, LOAD_LIBRARY_AS_DATAFILE);
    if(original == nullptr || reconstructed == nullptr)
    {
        if(original != nullptr)
        {
            FreeLibrary(original);
        }
        if(reconstructed != nullptr)
        {
            FreeLibrary(reconstructed);
        }
        return 3;
    }

    bool equal = true;
    for(const ResourceIdentity &identity : resources)
    {
        if(!compare_resource(original, reconstructed, identity))
        {
            std::fprintf(stderr, "resource mismatch: type=%u name=%u language=%u\n", identity.type, identity.name, identity.language);
            equal = false;
        }
    }

    FreeLibrary(reconstructed);
    FreeLibrary(original);
    return equal ? 0 : 1;
}
