#include "scene_description.h"
#include <algorithm>
#include <cctype>
#include <limits>

namespace xtet
{
namespace
{

bool tokenize(const std::vector<std::uint8_t> &bytes, std::vector<std::string> &tokens)
{
    tokens.clear();
    std::size_t offset = 0;
    while(offset < bytes.size())
    {
        while(offset < bytes.size() && (bytes[offset] == 0x1a || std::isspace((unsigned char)bytes[offset]) != 0))
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
        const std::size_t start = offset;
        while(offset < bytes.size() && bytes[offset] != 0x1a && std::isspace((unsigned char)bytes[offset]) == 0 && bytes[offset] != '{' && bytes[offset] != '}' && bytes[offset] != ';')
            ++offset;
        if(start == offset)
            return false;
        tokens.emplace_back((const char *)bytes.data() + start, offset - start);
    }
    return true;
}

bool parse_integer(const std::string &token, std::int32_t &value)
{
    if(token.empty())
        return false;
    std::size_t offset = token[0] == '-' ? 1 : 0;
    if(offset == token.size())
        return false;
    std::int64_t parsed = 0;
    for(; offset < token.size(); ++offset)
    {
        if(token[offset] < '0' || token[offset] > '9')
            return false;
        parsed = parsed * 10 + token[offset] - '0';
        if(parsed > (std::int64_t)(std::numeric_limits<std::int32_t>::max)() + 1)
            return false;
    }
    if(token[0] == '-')
        parsed = -parsed;
    if(parsed < (std::numeric_limits<std::int32_t>::min)() || parsed > (std::numeric_limits<std::int32_t>::max)())
        return false;
    value = (std::int32_t)parsed;
    return true;
}

bool is_body_keyword(const std::string &token)
{
    return token == "SIZE" || token == "MAP_SIZE" || token == "TO" || token == "SHOW" || token == "VIEW" || token == "TRANSP" || token == "FILL" || token == "item" || token == "map"
        || token == "EMPTY" || token == "LOAD" || token == "CREATE";
}

class SceneParser
{
public:
    explicit SceneParser(const SfsArchive &archive)
        : archive_(archive)
    {
    }

    bool load(const std::vector<std::string> &root_scripts, SceneDescription &description)
    {
        SceneDescription loaded;
        for(const std::string &path : root_scripts)
        {
            std::vector<SceneNode> roots;
            if(!parseScript(path, roots))
                return false;
            loaded.roots.insert(loaded.roots.end(), std::make_move_iterator(roots.begin()), std::make_move_iterator(roots.end()));
        }
        description = std::move(loaded);
        return true;
    }

private:
    bool parseScript(const std::string &path, std::vector<SceneNode> &roots)
    {
        if(std::find(active_scripts_.begin(), active_scripts_.end(), path) != active_scripts_.end())
            return false;
        std::vector<std::uint8_t> bytes;
        std::vector<std::string> tokens;
        if(!archive_.read(path, bytes) || !tokenize(bytes, tokens))
            return false;
        active_scripts_.push_back(path);
        std::size_t offset = 0;
        while(offset < tokens.size())
        {
            SceneNode node;
            if(!parseNode(tokens, offset, path, node))
                return false;
            roots.push_back(std::move(node));
        }
        active_scripts_.pop_back();
        return true;
    }

    bool parsePoint(const std::vector<std::string> &tokens, std::size_t &offset, ScenePoint &point)
    {
        return offset + 2 <= tokens.size() && parse_integer(tokens[offset++], point.x) && parse_integer(tokens[offset++], point.y);
    }

    bool parseFlag(const std::vector<std::string> &tokens, std::size_t &offset, std::optional<bool> &flag)
    {
        std::int32_t value = 0;
        if(offset >= tokens.size() || !parse_integer(tokens[offset++], value) || (value != 0 && value != 1))
            return false;
        flag = value != 0;
        return true;
    }

    bool parseNode(const std::vector<std::string> &tokens, std::size_t &offset, const std::string &source_script, SceneNode &node)
    {
        if(offset >= tokens.size() || tokens[offset++] != "{" || offset >= tokens.size())
            return false;
        const std::string type = tokens[offset++];
        node.source_script = source_script;
        if(type == "INCLUDE")
        {
            if(offset >= tokens.size())
                return false;
            std::vector<SceneNode> included;
            if(!parseScript(tokens[offset++], included) || included.size() != 1 || offset >= tokens.size() || tokens[offset++] != "}")
                return false;
            node = std::move(included.front());
            return parseLinks(tokens, offset, node);
        }
        if(type == "TSprites")
            node.type = SceneNodeType::sprites;
        else if(type == "TSprBmp")
            node.type = SceneNodeType::sprite_bitmap;
        else if(type == "TBmp")
            node.type = SceneNodeType::bitmap;
        else if(type == "TWave")
            node.type = SceneNodeType::wave;
        else
            return false;

        if(node.type == SceneNodeType::wave)
        {
            if(offset >= tokens.size() || tokens[offset] == "}")
                return false;
            node.loaded_path = tokens[offset++];
        }
        while(offset < tokens.size() && tokens[offset] != "}")
        {
            const std::string property = tokens[offset++];
            if(property == "SIZE" || property == "MAP_SIZE" || property == "TO")
            {
                ScenePoint point;
                if(!parsePoint(tokens, offset, point))
                    return false;
                if(property == "SIZE")
                    node.size = point;
                else if(property == "MAP_SIZE")
                    node.map_size = point;
                else
                    node.position = point;
            }
            else if(property == "SHOW")
            {
                if(!parseFlag(tokens, offset, node.shown))
                    return false;
            }
            else if(property == "VIEW")
            {
                if(!parseFlag(tokens, offset, node.viewed))
                    return false;
            }
            else if(property == "TRANSP")
            {
                if(!parseFlag(tokens, offset, node.transparent))
                    return false;
            }
            else if(property == "FILL")
            {
                std::int32_t value = 0;
                if(offset >= tokens.size() || !parse_integer(tokens[offset++], value) || value < 0 || value > 255)
                    return false;
                node.fill_index = (std::uint8_t)value;
            }
            else if(property == "item" || property == "map")
            {
                SceneNode child;
                if(!parseNode(tokens, offset, source_script, child))
                    return false;
                node.children.push_back(std::move(child));
            }
            else if(property == "EMPTY")
            {
                SceneNode child;
                child.type = SceneNodeType::empty;
                child.source_script = source_script;
                node.children.push_back(std::move(child));
            }
            else if(property == "LOAD")
            {
                if(node.type != SceneNodeType::bitmap || offset >= tokens.size() || !node.loaded_path.empty())
                    return false;
                node.loaded_path = tokens[offset++];
            }
            else if(property == "CREATE")
            {
                ScenePoint point;
                if(node.type != SceneNodeType::bitmap || node.created_size || !parsePoint(tokens, offset, point))
                    return false;
                node.created_size = point;
            }
            else
                return false;
        }
        if(offset >= tokens.size() || tokens[offset++] != "}")
            return false;
        if(node.type == SceneNodeType::bitmap && (node.loaded_path.empty() == !node.created_size))
            return false;
        if(node.type == SceneNodeType::wave && (!node.children.empty() || node.loaded_path.empty()))
            return false;
        return parseLinks(tokens, offset, node);
    }

    bool parseLinks(const std::vector<std::string> &tokens, std::size_t &offset, SceneNode &node)
    {
        while(offset < tokens.size() && tokens[offset] != "{" && tokens[offset] != "}" && !is_body_keyword(tokens[offset]))
            node.links.push_back(tokens[offset++]);
        return true;
    }

    const SfsArchive &archive_;
    std::vector<std::string> active_scripts_;
};

} // namespace

bool load_scene_description(const SfsArchive &archive, const std::vector<std::string> &root_scripts, SceneDescription &description)
{
    return SceneParser(archive).load(root_scripts, description);
}

std::vector<const SceneNode *> find_scene_links(const SceneDescription &description, const std::string &link)
{
    std::vector<const SceneNode *> matches;
    std::vector<const SceneNode *> pending;
    for(const SceneNode &root : description.roots)
        pending.push_back(&root);
    while(!pending.empty())
    {
        const SceneNode *node = pending.back();
        pending.pop_back();
        if(std::find(node->links.begin(), node->links.end(), link) != node->links.end())
            matches.push_back(node);
        for(const SceneNode &child : node->children)
            pending.push_back(&child);
    }
    return matches;
}

} // namespace xtet
