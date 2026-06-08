#include "search_cache.h"

namespace
{
    const char kHotKeywordsKey[] = "search:hot_keywords";
    const char kResultKeyPrefix[] = "search:result:";
} // namespace

namespace ns_redis
{
    SearchCache::SearchCache(const RedisConfig &config)
        : client_(config),
          ttl_seconds_(config.cache_ttl_seconds)
    {
    }

    CacheLookupStatus SearchCache::LookupResult(
        const std::string &keyword,
        std::string *result)
    {
        const RedisGetStatus status = client_.Get(ResultKey(keyword), result);
        if (status == RedisGetStatus::Hit)
        {
            return CacheLookupStatus::Hit;
        }
        if (status == RedisGetStatus::Miss)
        {
            return CacheLookupStatus::Miss;
        }
        return CacheLookupStatus::Unavailable;
    }

    bool SearchCache::StoreResult(
        const std::string &keyword,
        const std::string &result)
    {
        return client_.SetEx(ResultKey(keyword), ttl_seconds_, result);
    }

    bool SearchCache::RecordKeyword(const std::string &keyword)
    {
        return client_.ZIncrBy(kHotKeywordsKey, keyword);
    }

    bool SearchCache::GetHotKeywords(
        std::size_t limit,
        std::vector<std::pair<std::string, double> > *keywords)
    {
        return client_.ZRevRangeWithScores(kHotKeywordsKey, limit, keywords);
    }

    std::string SearchCache::ResultKey(const std::string &keyword) const
    {
        return std::string(kResultKeyPrefix) + keyword;
    }
} // namespace ns_redis
