#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <print>
#include <stddef.h>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <vector>
#include "../../src/portable_path.h"



namespace
{

struct CoffHeader
{
    uint16_t machine;
    uint16_t section_count;
    uint32_t timestamp;
    uint32_t symbol_table;
    uint32_t symbol_count;
    uint16_t optional_header_size;
    uint16_t characteristics;
};


struct DataDirectory
{
    uint32_t rva;
    uint32_t size;
};


struct OptionalHeader32
{
    uint16_t magic;
    uint8_t major_linker_version;
    uint8_t minor_linker_version;
    uint32_t code_size;
    uint32_t initialized_data_size;
    uint32_t uninitialized_data_size;
    uint32_t entry_point;
    uint32_t code_base;
    uint32_t data_base;
    uint32_t image_base;
    uint32_t section_alignment;
    uint32_t file_alignment;
    uint16_t major_os_version;
    uint16_t minor_os_version;
    uint16_t major_image_version;
    uint16_t minor_image_version;
    uint16_t major_subsystem_version;
    uint16_t minor_subsystem_version;
    uint32_t win32_version;
    uint32_t image_size;
    uint32_t headers_size;
    uint32_t checksum;
    uint16_t subsystem;
    uint16_t dll_characteristics;
    uint32_t stack_reserve_size;
    uint32_t stack_commit_size;
    uint32_t heap_reserve_size;
    uint32_t heap_commit_size;
    uint32_t loader_flags;
    uint32_t data_directory_count;
    DataDirectory data_directories[16];
};


struct SectionHeader
{
    char name[8];
    uint32_t virtual_size;
    uint32_t virtual_address;
    uint32_t raw_size;
    uint32_t raw_offset;
    uint32_t relocations;
    uint32_t line_numbers;
    uint16_t relocation_count;
    uint16_t line_number_count;
    uint32_t characteristics;
};


struct ResourceDirectoryHeader
{
    uint32_t characteristics;
    uint32_t timestamp;
    uint16_t major_version;
    uint16_t minor_version;
    uint16_t named_count;
    uint16_t id_count;
};


struct DirectoryEntry
{
    uint32_t name;
    uint32_t target;
};


struct ResourceDataEntry
{
    uint32_t rva;
    uint32_t size;
    uint32_t code_page;
    uint32_t reserved;
};


std::vector<uint8_t> extract_sfs(const std::string &source)
{
    std::filesystem::path resolved_source;
    if(!freegag::resolve_existing_host_path_case_insensitive(source, &resolved_source))
        throw std::runtime_error("could not open input DLL");
    std::ifstream ifs(resolved_source, std::ios::binary);
    if(!ifs)
        throw std::runtime_error("could not open input DLL");

    uint16_t mz_signature;
    ifs.read((char *)&mz_signature, sizeof(mz_signature));
    if(!ifs || mz_signature != 0x5a4d)
        throw std::runtime_error("input is not an MZ executable");

    uint32_t pe_offset;
    ifs.seekg(0x3c);
    ifs.read((char *)&pe_offset, sizeof(pe_offset));
    if(!ifs)
        throw std::runtime_error("could not read PE header offset");

    uint32_t pe_signature;
    ifs.seekg(pe_offset);
    ifs.read((char *)&pe_signature, sizeof(pe_signature));
    if(!ifs || pe_signature != 0x00004550)
        throw std::runtime_error("input has no PE signature");

    CoffHeader coff;
    ifs.read((char *)&coff, sizeof(coff));
    if(!ifs)
        throw std::runtime_error("could not read PE header");
    if(coff.optional_header_size != sizeof(OptionalHeader32))
        throw std::runtime_error("input has an unsupported PE optional header");

    OptionalHeader32 optional;
    ifs.read((char *)&optional, sizeof(optional));
    if(!ifs || optional.magic != 0x10b || optional.data_directory_count <= 2)
        throw std::runtime_error("input has an unsupported PE optional header");

    const DataDirectory &resource = optional.data_directories[2];
    if(resource.rva == 0 || resource.size == 0)
        throw std::runtime_error("input DLL has no resource directory");

    std::vector<SectionHeader> sections(coff.section_count);
    ifs.read((char *)sections.data(), sections.size() * sizeof(SectionHeader));
    if(!ifs)
        throw std::runtime_error("could not read PE section headers");

    const SectionHeader *resource_section = nullptr;
    for(const SectionHeader &section : sections)
    {
        const uint64_t extent = std::max(section.virtual_size, section.raw_size);
        if(resource.rva >= section.virtual_address && resource.rva < (uint64_t)section.virtual_address + extent)
        {
            resource_section = &section;
            break;
        }
    }
    if(!resource_section)
        throw std::runtime_error("the PE resource RVA does not map to a section");

    const uint64_t resource_offset = (uint64_t)resource_section->raw_offset + resource.rva - resource_section->virtual_address;

    ResourceDirectoryHeader type_header;
    ifs.seekg(resource_offset);
    ifs.read((char *)&type_header, sizeof(type_header));
    if(!ifs)
        throw std::runtime_error("could not read PE resource type directory");

    std::vector<DirectoryEntry> types((size_t)type_header.named_count + type_header.id_count);
    ifs.read((char *)types.data(), types.size() * sizeof(DirectoryEntry));
    if(!ifs)
        throw std::runtime_error("could not read PE resource type directory");

    std::vector<uint8_t> payload;
    size_t match_count = 0;
    for(const DirectoryEntry &type : types)
    {
        if((type.name & (1u << 31)) != 0 || type.name != 10 || (type.target & (1u << 31)) == 0)
            continue;

        ResourceDirectoryHeader name_header;
        ifs.seekg(resource_offset + (type.target & ~(1u << 31)));
        ifs.read((char *)&name_header, sizeof(name_header));
        if(!ifs)
            throw std::runtime_error("could not read PE resource name directory");

        std::vector<DirectoryEntry> names((size_t)name_header.named_count + name_header.id_count);
        ifs.read((char *)names.data(), names.size() * sizeof(DirectoryEntry));
        if(!ifs)
            throw std::runtime_error("could not read PE resource name directory");

        for(const DirectoryEntry &name : names)
        {
            if((name.name & (1u << 31)) == 0 || (name.target & (1u << 31)) == 0)
                continue;

            uint16_t name_length;
            ifs.seekg(resource_offset + (name.name & ~(1u << 31)));
            ifs.read((char *)&name_length, sizeof(name_length));
            if(!ifs)
                throw std::runtime_error("could not read PE resource name length");

            std::u16string resource_name(name_length, u'\0');
            ifs.read((char *)resource_name.data(), resource_name.size() * sizeof(char16_t));
            if(!ifs)
                throw std::runtime_error("could not read PE resource name");

            if(resource_name != u"XTETSFS")
                continue;

            ResourceDirectoryHeader language_header;
            ifs.seekg(resource_offset + (name.target & ~(1u << 31)));
            ifs.read((char *)&language_header, sizeof(language_header));
            if(!ifs)
                throw std::runtime_error("could not read PE resource language directory");

            std::vector<DirectoryEntry> languages((size_t)language_header.named_count + language_header.id_count);
            ifs.read((char *)languages.data(), languages.size() * sizeof(DirectoryEntry));
            if(!ifs)
                throw std::runtime_error("could not read PE resource language directory");

            for(const DirectoryEntry &language : languages)
            {
                if((language.target & (1u << 31)) != 0)
                    throw std::runtime_error("XTETSFS has an invalid language directory");

                ResourceDataEntry data;
                ifs.seekg(resource_offset + language.target);
                ifs.read((char *)&data, sizeof(data));
                if(!ifs)
                    throw std::runtime_error("could not read embedded resource entry");

                const SectionHeader *data_section = nullptr;
                for(const SectionHeader &section : sections)
                {
                    const uint64_t extent = std::max(section.virtual_size, section.raw_size);
                    if(data.rva >= section.virtual_address && data.rva < (uint64_t)section.virtual_address + extent)
                    {
                        data_section = &section;
                        break;
                    }
                }
                if(!data_section)
                    throw std::runtime_error("the embedded resource RVA does not map to a section");

                const uint64_t data_offset = (uint64_t)data_section->raw_offset + data.rva - data_section->virtual_address;

                payload.resize(data.size);
                ifs.seekg(data_offset);
                ifs.read((char *)payload.data(), payload.size());
                if(!ifs)
                    throw std::runtime_error("could not read embedded resource");

                ++match_count;
            }
        }
    }
    if(match_count != 1)
        throw std::runtime_error("expected exactly one XTETSFS RCDATA resource, found " + std::to_string(match_count));

    return payload;
}


void extract(const std::string &destination, const std::string &source)
{
    const std::vector<uint8_t> payload = extract_sfs(source);

    std::ofstream ofs(destination, std::ios::binary);
    ofs.write((const char *)payload.data(), payload.size());
    if(!ofs)
        throw std::runtime_error("could not write " + destination);
}

}


int main(int argc, char **argv)
{
    if(argc != 2)
    {
        std::println(stderr, "usage: xtet_resource_extractor XTETDLL.DLL");

        return 2;
    }

    try
    {
        extract(std::filesystem::path(argv[1]).stem().string() + ".SFS", argv[1]);

        return 0;
    }
    catch(const std::exception &e)
    {
        std::println(stderr, "error: {}", e.what());

        return 1;
    }
}
