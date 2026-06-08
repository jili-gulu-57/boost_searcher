#include "../config.h"
#include "../search_cache.h"

#include <chrono>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace
{
    const char *LookupStatusName(ns_redis::CacheLookupStatus status)
    {
        if (status == ns_redis::CacheLookupStatus::Hit)
        {
            return "HIT";
        }
        if (status == ns_redis::CacheLookupStatus::Miss)
        {
            return "MISS";
        }
        return "BYPASS";
    }
} // namespace

int main()
{
    const ns_redis::RedisConfig config = ns_redis::RedisConfig::FromEnvironment();
    ns_redis::SearchCache cache(config);

    const long long timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    const std::string keyword = "redis-demo-" + std::to_string(timestamp);
    const std::string result = R"([{"title":"redis cache demo"}])";

    std::string cached;
    ns_redis::CacheLookupStatus first = cache.LookupResult(keyword, &cached);
    std::cout << "first query: cache " << LookupStatusName(first) << std::endl;
    if (first == ns_redis::CacheLookupStatus::Unavailable)
    {
        std::cout << "Redis is unavailable; local-search fallback would be used." << std::endl;
        return 0;
    }

    cache.RecordKeyword(keyword);
    cache.StoreResult(keyword, result);

    cached.clear();
    ns_redis::CacheLookupStatus second = cache.LookupResult(keyword, &cached);
    cache.RecordKeyword(keyword);
    std::cout << "second query: cache " << LookupStatusName(second) << std::endl;
    std::cout << "cached result: " << cached << std::endl;

    cache.RecordKeyword("boost");
    cache.RecordKeyword("boost");
    cache.RecordKeyword("cpp");

    std::vector<std::pair<std::string, double> > hot_keywords;
    if (cache.GetHotKeywords(10, &hot_keywords))
    {
        std::cout << "hot keywords:" << std::endl;
        for (const auto &item : hot_keywords)
        {
            std::cout << "  " << item.first << " => " << item.second << std::endl;
        }
    }
    return second == ns_redis::CacheLookupStatus::Hit ? 0 : 1;
}
