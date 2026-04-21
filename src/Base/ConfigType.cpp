#include "ConfigType.h"

namespace Base
{
    const char *typeName(const ConfigValueType type) noexcept
    {
        switch (type)
        {
            case ConfigValueType::Null: return "null";
            case ConfigValueType::Bool: return "bool";
            case ConfigValueType::Int: return "int";
            case ConfigValueType::Double: return "double";
            case ConfigValueType::String: return "string";
            case ConfigValueType::Array: return "array";
            case ConfigValueType::Object: return "object";
            default: return "unknown";
        }
    }

    bool isYamlFile(const std::string_view file_path) noexcept
    {
        constexpr std::string_view YAML_EXT = ".yaml";
        constexpr std::string_view YML_EXT = ".yml";

        if (file_path.length() < YML_EXT.length())
        {
            return false;
        }

        if (file_path.length() >= YAML_EXT.length() &&
            file_path.substr(file_path.length() - YAML_EXT.length()) == YAML_EXT)
        {
            return true;
        }

        return file_path.substr(file_path.length() - YML_EXT.length()) == YML_EXT;
    }

    std::vector<std::string> splitKey(std::string_view key, const char delimiter)
    {
        std::vector<std::string> parts;
        if (key.empty())
        {
            return parts;
        }

        size_t start = 0;
        size_t end = key.find(delimiter);

        while (end != std::string_view::npos)
        {
            if (end > start)
            {
                parts.emplace_back(key.substr(start, end - start));
            }
            start = end + 1;
            end = key.find(delimiter, start);
        }

        if (start < key.length())
        {
            parts.emplace_back(key.substr(start));
        }

        return parts;
    }
}
