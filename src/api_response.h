#pragma once

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

using ApiJson = nlohmann::json;

// OpenAI-compatible servers disagree on whether function.arguments is a JSON
// string or an already-decoded JSON value. Ollama may return either shape.
inline ApiJson parseOpenAIFunctionArguments(const ApiJson& function)
{
    if (!function.contains("arguments") || function["arguments"].is_null())
        return ApiJson::object();

    const auto& arguments = function["arguments"];
    if (arguments.is_object())
        return arguments;

    if (arguments.is_string())
    {
        try
        {
            ApiJson parsed = ApiJson::parse(arguments.get<std::string>());
            if (!parsed.is_object())
                throw std::runtime_error("tool arguments must decode to a JSON object");
            return parsed;
        }
        catch (const ApiJson::exception& e)
        {
            throw std::runtime_error(std::string("Invalid JSON in tool arguments: ") + e.what());
        }
    }

    throw std::runtime_error("tool arguments must be a JSON object or JSON-encoded object");
}

inline ApiJson normalizeOpenAIResponse(const ApiJson& raw)
{
    if (!raw.contains("choices") || !raw["choices"].is_array() || raw["choices"].empty() ||
        !raw["choices"][0].contains("message") || !raw["choices"][0]["message"].is_object())
        throw std::runtime_error("OpenAI-compatible response has no choices[0].message");

    const auto& message = raw["choices"][0]["message"];
    ApiJson content = ApiJson::array();

    if (message.contains("content") && !message["content"].is_null())
    {
        if (!message["content"].is_string())
            throw std::runtime_error("OpenAI-compatible message.content must be a string or null");
        content.push_back({{"type", "text"}, {"text", message["content"]}});
    }

    if (message.contains("tool_calls") && !message["tool_calls"].is_null())
    {
        if (!message["tool_calls"].is_array())
            throw std::runtime_error("OpenAI-compatible message.tool_calls must be an array");

        for (const auto& tc : message["tool_calls"])
        {
            if (!tc.contains("id") || !tc["id"].is_string() ||
                !tc.contains("function") || !tc["function"].is_object() ||
                !tc["function"].contains("name") || !tc["function"]["name"].is_string())
                throw std::runtime_error("OpenAI-compatible tool call is missing id or function.name");

            const auto& function = tc["function"];
            content.push_back({
                {"type", "tool_use"},
                {"id", tc["id"]},
                {"name", function["name"]},
                {"input", parseOpenAIFunctionArguments(function)}
            });
        }
    }

    return {{"role", "assistant"}, {"content", content}};
}
