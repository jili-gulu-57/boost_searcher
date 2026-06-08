#pragma once

#include <string>

namespace ns_redis
{
    struct RedisConfig
    {
        bool enabled;
        std::string host;
        int port;
        std::string password;
        int db;
        int cache_ttl_seconds;
        int connect_timeout_ms;
        int socket_timeout_ms;

        static RedisConfig FromEnvironment();
    };
} // namespace ns_redis
