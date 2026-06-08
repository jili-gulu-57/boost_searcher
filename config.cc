#include "config.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace
{
    std::string GetEnv(const char *name, const std::string &fallback)
    {
        const char *value = std::getenv(name);
        return value && *value ? value : fallback;
    }

    int GetPositiveIntEnv(const char *name, int fallback)
    {
        const char *value = std::getenv(name);
        if (!value || !*value)
        {
            return fallback;
        }

        try
        {
            const int parsed = std::stoi(value);
            return parsed >= 0 ? parsed : fallback;
        }
        catch (...)
        {
            return fallback;
        }
    }

    bool GetBoolEnv(const char *name, bool fallback)
    {
        std::string value = GetEnv(name, fallback ? "true" : "false");
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch)
                       { return static_cast<char>(std::tolower(ch)); });
        if (value == "1" || value == "true" || value == "yes" || value == "on")
        {
            return true;
        }
        if (value == "0" || value == "false" || value == "no" || value == "off")
        {
            return false;
        }
        return fallback;
    }
} // namespace

namespace ns_redis
{
    RedisConfig RedisConfig::FromEnvironment()
    {
        RedisConfig config;
        config.enabled = GetBoolEnv("REDIS_ENABLED", true);
        config.host = GetEnv("REDIS_HOST", "127.0.0.1");
        config.port = std::max(1, GetPositiveIntEnv("REDIS_PORT", 6379));
        config.password = GetEnv("REDIS_PASSWORD", "");
        config.db = GetPositiveIntEnv("REDIS_DB", 0);
        config.cache_ttl_seconds = std::max(1, GetPositiveIntEnv("REDIS_CACHE_TTL", 300));
        config.connect_timeout_ms = std::max(1, GetPositiveIntEnv("REDIS_CONNECT_TIMEOUT_MS", 300));
        config.socket_timeout_ms = std::max(1, GetPositiveIntEnv("REDIS_SOCKET_TIMEOUT_MS", 500));
        return config;
    }
} // namespace ns_redis
