#pragma once

#include "config.h"

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ns_redis
{
    enum class RedisGetStatus
    {
        Hit,
        Miss,
        Error
    };

    class RedisClient
    {
    public:
        explicit RedisClient(const RedisConfig &config);
        ~RedisClient();

        RedisGetStatus Get(const std::string &key, std::string *value);
        bool SetEx(const std::string &key, int ttl_seconds, const std::string &value);
        bool ZIncrBy(const std::string &key, const std::string &member);
        bool ZRevRangeWithScores(
            const std::string &key,
            std::size_t limit,
            std::vector<std::pair<std::string, double> > *items);

        bool IsEnabled() const;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
} // namespace ns_redis
