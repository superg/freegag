#include "asset_manifest.h"
#include <algorithm>
#include <cctype>
#include <stdint.h>

namespace xtet
{

constexpr uint8_t DOS_TEXT_END = 0x1a;

bool tokenize_asset_manifest(const std::vector<uint8_t> &bytes, std::vector<std::string> &tokens)
{
    tokens.clear();
    size_t offset = 0;
    while(offset < bytes.size())
    {
        while(offset < bytes.size() && (bytes[offset] == DOS_TEXT_END || std::isspace(static_cast<unsigned char>(bytes[offset])) != 0))
            ++offset;
        if(offset == bytes.size())
            break;
        if(bytes[offset] == ';')
        {
            while(offset < bytes.size() && bytes[offset] != '\r' && bytes[offset] != '\n')
                ++offset;
            continue;
        }
        if(bytes[offset] == '{' || bytes[offset] == '}')
        {
            tokens.emplace_back(1, (char)bytes[offset++]);
            continue;
        }
        const size_t start = offset;
        while(offset < bytes.size() && bytes[offset] != DOS_TEXT_END && std::isspace(static_cast<unsigned char>(bytes[offset])) == 0 && bytes[offset] != '{' && bytes[offset] != '}'
              && bytes[offset] != ';')
            ++offset;
        if(start == offset)
            return false;
        tokens.emplace_back((const char *)bytes.data() + start, offset - start);
    }
    return true;
}

bool contains(const std::vector<std::string> &values, const std::string &value)
{
    return std::find(values.begin(), values.end(), value) != values.end();
}

bool load_script(const SfsArchive &archive, const std::string &path, AssetManifest &manifest, std::vector<std::string> &active_scripts)
{
    if(contains(active_scripts, path))
        return false;
    if(contains(manifest.script_paths, path))
        return true;

    std::vector<uint8_t> bytes;
    std::vector<std::string> tokens;
    if(!archive.read(path, bytes) || !tokenize_asset_manifest(bytes, tokens))
        return false;

    active_scripts.push_back(path);
    manifest.script_paths.push_back(path);
    int depth = 0;
    for(size_t index = 0; index < tokens.size(); ++index)
    {
        const std::string &token = tokens[index];
        if(token == "{")
        {
            ++depth;
            continue;
        }
        if(token == "}")
        {
            if(--depth < 0)
                return false;
            continue;
        }
        if(token == "INCLUDE")
        {
            if(index + 1 >= tokens.size() || !load_script(archive, tokens[++index], manifest, active_scripts))
                return false;
            continue;
        }
        if(token == "LOAD")
        {
            if(index + 1 >= tokens.size())
                return false;
            manifest.bitmap_paths.push_back(tokens[++index]);
            continue;
        }
        if(token == "TWave")
        {
            if(index + 1 >= tokens.size() || tokens[index + 1] == "{" || tokens[index + 1] == "}")
                return false;
            manifest.wave_paths.push_back(tokens[++index]);
        }
    }
    active_scripts.pop_back();
    return depth == 0;
}


bool load_asset_manifest(const SfsArchive &archive, const std::vector<std::string> &root_scripts, AssetManifest &manifest)
{
    AssetManifest loaded;
    std::vector<std::string> active_scripts;
    for(const std::string &path : root_scripts)
        if(!load_script(archive, path, loaded, active_scripts))
            return false;
    for(const std::string &path : loaded.bitmap_paths)
        if(!archive.find(path))
            return false;
    for(const std::string &path : loaded.wave_paths)
        if(!archive.find(path))
            return false;
    manifest = std::move(loaded);
    return true;
}

} // namespace xtet
