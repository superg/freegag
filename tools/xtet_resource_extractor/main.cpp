#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace
{

class Reader
{
public:
    explicit Reader(std::vector<std::uint8_t> bytes)
        : bytes_(std::move(bytes))
    {
    }

    template<typename T>
    T read(std::size_t offset) const
    {
        require(offset, sizeof(T));
        T value{};
        std::memcpy(&value, bytes_.data() + offset, sizeof(value));
        return value;
    }

    std::vector<std::uint8_t> slice(std::size_t offset, std::size_t size) const
    {
        require(offset, size);
        return std::vector<std::uint8_t>(bytes_.begin() + (std::ptrdiff_t)offset, bytes_.begin() + (std::ptrdiff_t)(offset + size));
    }

    std::size_t size() const
    {
        return bytes_.size();
    }

private:
    void require(std::size_t offset, std::size_t size) const
    {
        if(offset > bytes_.size() || size > bytes_.size() - offset)
            throw std::runtime_error("the PE file contains an out-of-bounds reference");
    }

    std::vector<std::uint8_t> bytes_;
};

#pragma pack(push, 1)
struct CoffHeader
{
    std::uint16_t machine;
    std::uint16_t section_count;
    std::uint32_t timestamp;
    std::uint32_t symbol_table;
    std::uint32_t symbol_count;
    std::uint16_t optional_header_size;
    std::uint16_t characteristics;
};

struct SectionHeader
{
    char name[8];
    std::uint32_t virtual_size;
    std::uint32_t virtual_address;
    std::uint32_t raw_size;
    std::uint32_t raw_offset;
    std::uint32_t relocations;
    std::uint32_t line_numbers;
    std::uint16_t relocation_count;
    std::uint16_t line_number_count;
    std::uint32_t characteristics;
};
#pragma pack(pop)

static_assert(sizeof(CoffHeader) == 20);
static_assert(sizeof(SectionHeader) == 40);

struct PeImage
{
    Reader reader;
    std::vector<SectionHeader> sections;
    std::uint32_t resource_rva{};
    std::uint32_t resource_size{};
    std::size_t resource_offset{};
};

std::vector<std::uint8_t> read_file(const std::filesystem::path &path)
{
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if(!input)
        throw std::runtime_error("could not open input DLL");
    const std::streamoff length = input.tellg();
    if(length < 0 || (std::uintmax_t)length > (std::uintmax_t)std::numeric_limits<std::size_t>::max())
        throw std::runtime_error("input DLL is too large");
    std::vector<std::uint8_t> bytes((std::size_t)length);
    input.seekg(0);
    if(!bytes.empty() && !input.read((char *)bytes.data(), length))
        throw std::runtime_error("could not read input DLL");
    return bytes;
}

std::size_t rva_to_offset(const Reader &reader, const std::vector<SectionHeader> &sections, std::uint32_t rva, std::size_t size)
{
    for(const SectionHeader &section : sections)
    {
        const std::uint64_t extent = std::max(section.virtual_size, section.raw_size);
        if(rva < section.virtual_address || (std::uint64_t)rva + size > (std::uint64_t)section.virtual_address + extent)
            continue;
        const std::uint64_t offset = (std::uint64_t)section.raw_offset + rva - section.virtual_address;
        if(offset + size > reader.size())
            throw std::runtime_error("a PE RVA maps outside the input file");
        return (std::size_t)offset;
    }
    throw std::runtime_error("a PE RVA does not map to a section");
}

PeImage parse_pe(std::vector<std::uint8_t> bytes)
{
    Reader reader(std::move(bytes));
    if(reader.read<std::uint16_t>(0) != 0x5a4d)
        throw std::runtime_error("input is not an MZ executable");
    const std::uint32_t pe_offset = reader.read<std::uint32_t>(0x3c);
    if(reader.read<std::uint32_t>(pe_offset) != 0x00004550)
        throw std::runtime_error("input has no PE signature");

    const CoffHeader coff = reader.read<CoffHeader>((std::size_t)pe_offset + 4);
    const std::size_t optional_offset = (std::size_t)pe_offset + 4 + sizeof(CoffHeader);
    const std::uint16_t magic = reader.read<std::uint16_t>(optional_offset);
    const std::size_t directory_offset = magic == 0x10b ? 96 : magic == 0x20b ? 112 : 0;
    if(directory_offset == 0 || coff.optional_header_size < directory_offset + 8 * 3)
        throw std::runtime_error("input has an unsupported PE optional header");
    const std::uint32_t resource_rva = reader.read<std::uint32_t>(optional_offset + directory_offset + 8 * 2);
    const std::uint32_t resource_size = reader.read<std::uint32_t>(optional_offset + directory_offset + 8 * 2 + 4);
    if(resource_rva == 0 || resource_size == 0)
        throw std::runtime_error("input DLL has no resource directory");

    std::vector<SectionHeader> sections;
    const std::size_t sections_offset = optional_offset + coff.optional_header_size;
    for(std::uint16_t index = 0; index < coff.section_count; ++index)
        sections.push_back(reader.read<SectionHeader>(sections_offset + (std::size_t)index * sizeof(SectionHeader)));
    const std::size_t resource_offset = rva_to_offset(reader, sections, resource_rva, resource_size);
    return PeImage{ std::move(reader), std::move(sections), resource_rva, resource_size, resource_offset };
}

std::u16string resource_name(const PeImage &image, std::uint32_t value)
{
    const std::size_t offset = image.resource_offset + (value & 0x7fffffff);
    const std::uint16_t length = image.reader.read<std::uint16_t>(offset);
    std::u16string name;
    name.reserve(length);
    for(std::uint16_t index = 0; index < length; ++index)
        name.push_back((char16_t)image.reader.read<std::uint16_t>(offset + 2 + (std::size_t)index * 2));
    return name;
}

struct DirectoryEntry
{
    std::uint32_t name;
    std::uint32_t target;
};

std::vector<DirectoryEntry> directory_entries(const PeImage &image, std::uint32_t relative_offset)
{
    const std::size_t offset = image.resource_offset + relative_offset;
    const std::uint16_t named_count = image.reader.read<std::uint16_t>(offset + 12);
    const std::uint16_t id_count = image.reader.read<std::uint16_t>(offset + 14);
    const std::size_t total = (std::size_t)named_count + id_count;
    std::vector<DirectoryEntry> entries;
    entries.reserve(total);
    for(std::size_t index = 0; index < total; ++index)
    {
        const std::size_t entry_offset = offset + 16 + index * 8;
        entries.push_back({ image.reader.read<std::uint32_t>(entry_offset), image.reader.read<std::uint32_t>(entry_offset + 4) });
    }
    return entries;
}

std::vector<std::uint8_t> extract_xtetsfs(const PeImage &image)
{
    std::vector<std::uint8_t> matches;
    std::size_t match_count = 0;
    for(const DirectoryEntry &type : directory_entries(image, 0))
    {
        if((type.name & 0x80000000) != 0 || type.name != 10 || (type.target & 0x80000000) == 0)
            continue;
        for(const DirectoryEntry &name : directory_entries(image, type.target & 0x7fffffff))
        {
            if((name.name & 0x80000000) == 0 || resource_name(image, name.name) != u"XTETSFS" || (name.target & 0x80000000) == 0)
                continue;
            for(const DirectoryEntry &language : directory_entries(image, name.target & 0x7fffffff))
            {
                if((language.target & 0x80000000) != 0)
                    throw std::runtime_error("XTETSFS has an invalid language directory");
                const std::size_t data_offset = image.resource_offset + language.target;
                const std::uint32_t data_rva = image.reader.read<std::uint32_t>(data_offset);
                const std::uint32_t data_size = image.reader.read<std::uint32_t>(data_offset + 4);
                matches = image.reader.slice(rva_to_offset(image.reader, image.sections, data_rva, data_size), data_size);
                ++match_count;
            }
        }
    }
    if(match_count != 1)
        throw std::runtime_error("expected exactly one XTETSFS RCDATA resource, found " + std::to_string(match_count));
    return matches;
}

// Compact SHA-256 implementation used to make extracted evidence self-verifying.
std::string sha256(const std::vector<std::uint8_t> &bytes)
{
    constexpr std::array<std::uint32_t, 64> constants{ 0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2 };
    std::vector<std::uint8_t> padded = bytes;
    const std::uint64_t bit_count = (std::uint64_t)bytes.size() * 8;
    padded.push_back(0x80);
    while((padded.size() % 64) != 56)
        padded.push_back(0);
    for(int shift = 56; shift >= 0; shift -= 8)
        padded.push_back((std::uint8_t)(bit_count >> shift));
    std::array<std::uint32_t, 8> hash{ 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
    auto rotate = [](std::uint32_t value, int amount) { return (value >> amount) | (value << (32 - amount)); };
    for(std::size_t block = 0; block < padded.size(); block += 64)
    {
        std::array<std::uint32_t, 64> words{};
        for(std::size_t index = 0; index < 16; ++index)
            words[index] = ((std::uint32_t)padded[block + index * 4] << 24) | ((std::uint32_t)padded[block + index * 4 + 1] << 16) | ((std::uint32_t)padded[block + index * 4 + 2] << 8)
                         | padded[block + index * 4 + 3];
        for(std::size_t index = 16; index < words.size(); ++index)
        {
            const std::uint32_t a = rotate(words[index - 15], 7) ^ rotate(words[index - 15], 18) ^ (words[index - 15] >> 3);
            const std::uint32_t b = rotate(words[index - 2], 17) ^ rotate(words[index - 2], 19) ^ (words[index - 2] >> 10);
            words[index] = words[index - 16] + a + words[index - 7] + b;
        }
        std::array<std::uint32_t, 8> state = hash;
        for(std::size_t index = 0; index < words.size(); ++index)
        {
            const std::uint32_t sum1 = rotate(state[4], 6) ^ rotate(state[4], 11) ^ rotate(state[4], 25);
            const std::uint32_t choose = (state[4] & state[5]) ^ (~state[4] & state[6]);
            const std::uint32_t temp1 = state[7] + sum1 + choose + constants[index] + words[index];
            const std::uint32_t sum0 = rotate(state[0], 2) ^ rotate(state[0], 13) ^ rotate(state[0], 22);
            const std::uint32_t majority = (state[0] & state[1]) ^ (state[0] & state[2]) ^ (state[1] & state[2]);
            const std::uint32_t temp2 = sum0 + majority;
            state = { temp1 + temp2, state[0], state[1], state[2], state[3] + temp1, state[4], state[5], state[6] };
        }
        for(std::size_t index = 0; index < hash.size(); ++index)
            hash[index] += state[index];
    }
    std::ostringstream text;
    text << std::hex << std::setfill('0');
    for(std::uint32_t word : hash)
        text << std::setw(8) << word;
    return text.str();
}

void write_file(const std::filesystem::path &path, const void *data, std::size_t size)
{
    std::ofstream output(path, std::ios::binary);
    if(!output || (size != 0 && !output.write((const char *)data, (std::streamsize)size)))
        throw std::runtime_error("could not write " + path.string());
}

void extract(const std::filesystem::path &dll_path, const std::filesystem::path &output_path)
{
    if(std::filesystem::exists(output_path))
        throw std::runtime_error("output directory already exists");
    const std::vector<std::uint8_t> payload = extract_xtetsfs(parse_pe(read_file(dll_path)));
    const std::string digest = sha256(payload);
    const std::filesystem::path temporary_path = output_path.string() + ".tmp";
    if(std::filesystem::exists(temporary_path))
        throw std::runtime_error("temporary output directory already exists: " + temporary_path.string());
    std::filesystem::create_directories(temporary_path);
    try
    {
        write_file(temporary_path / "XTETSFS.bin", payload.data(), payload.size());
        const std::string metadata = "{\n  \"resource_name\": \"XTETSFS\",\n  \"resource_type\": 10,\n  \"size\": " + std::to_string(payload.size()) + ",\n  \"sha256\": \"" + digest + "\"\n}\n";
        write_file(temporary_path / "XTETSFS.json", metadata.data(), metadata.size());
        const std::string resource_script = "XTETSFS RCDATA \"XTETSFS.bin\"\n";
        write_file(temporary_path / "XTETSFS.rc", resource_script.data(), resource_script.size());
        std::filesystem::rename(temporary_path, output_path);
    }
    catch(...)
    {
        std::filesystem::remove_all(temporary_path);
        throw;
    }
    std::cout << "Extracted XTETSFS (" << payload.size() << " bytes, SHA-256 " << digest << ")\n";
}

} // namespace

int main(int argc, char **argv)
{
    if(argc != 3)
    {
        std::cerr << "Usage: xtet_resource_extractor <original-dll> <output-directory>\n";
        return 2;
    }
    try
    {
        extract(argv[1], argv[2]);
        return 0;
    }
    catch(const std::exception &error)
    {
        std::cerr << "xtet_resource_extractor: " << error.what() << '\n';
        return 1;
    }
}
