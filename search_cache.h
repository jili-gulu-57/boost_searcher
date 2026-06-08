#pragma once

#include "redis_client.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace ns_redis
{
    enum class CacheLookupStatus
    {
        Hit,
        Miss,
        Unavailable
    };

    class SearchCache
    {
    public:
        explicit SearchCache(const RedisConfig &config);

        CacheLookupStatus LookupResult(const std::string &keyword, std::string *result);
        bool StoreResult(const std::string &keyword, const std::string &result);
        bool RecordKeyword(const std::string &keyword);
        bool GetHotKeywords(
            std::size_t limit,
            std::vector<std::pair<std::string, double> > *keywords);

    private:
        std::string ResultKey(const std::string &keyword) const;

    private:
        RedisClient client_;
        int ttl_seconds_;
    };
} // namespace ns_redis
