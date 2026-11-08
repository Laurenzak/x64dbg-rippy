#pragma once

#include <string>

// Accept either an API base URL (for example /v1) or the full chat endpoint.
inline std::string openAIPathPrefix(std::string path)
{
    while (!path.empty() && path.back() == '/')
        path.pop_back();

    constexpr char endpoint[] = "/chat/completions";
    constexpr std::size_t endpointLength = sizeof(endpoint) - 1;
    if (path.size() >= endpointLength &&
        path.compare(path.size() - endpointLength, endpointLength, endpoint) == 0)
    {
        path.erase(path.size() - endpointLength);
    }
    return path;
}

inline std::string configuredModel(const std::string& model, const std::string& fallback)
{
    return model.empty() ? fallback : model;
}

