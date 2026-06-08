#pragma once

#include "index.hpp"
#include "search_cache.h"
#include "util.hpp"
#include <algorithm>
#include <jsoncpp/json/json.h>
#include <memory>
#include <unordered_map>

namespace ns_searcher
{
    enum class SearchCacheStatus
    {
        Bypass,
        Hit,
        Miss
    };

    inline const char *SearchCacheStatusName(SearchCacheStatus status)
    {
        if (status == SearchCacheStatus::Hit)
        {
            return "HIT";
        }
        if (status == SearchCacheStatus::Miss)
        {
            return "MISS";
        }
        return "BYPASS";
    }

    struct SearchMetadata
    {
        SearchCacheStatus cache_status;

        SearchMetadata() : cache_status(SearchCacheStatus::Bypass) {}
    };

    struct InvertedElemPrint
    {
        u_int64_t doc_id;
        int weight;
        std::vector<std::string> words;
        InvertedElemPrint() : doc_id(0), weight(0) {}
    };

    class Searcher
    {
    private:
        ns_index::Index *index;
        std::unique_ptr<ns_redis::SearchCache> search_cache;

    public:
        Searcher()
            : index(nullptr),
              search_cache(new ns_redis::SearchCache(ns_redis::RedisConfig::FromEnvironment()))
        {
        }
        ~Searcher() {}

        bool InitSearcher(const std::string &input)
        {
            // 1.创建index对象
            index = ns_index::Index::GetInstance();
            // std::cout << "建立Index对象成功" << std::endl;
            LOG(NORMAL, "建立Index对象成功");
            // 2.构建索引(将硬盘里的索引文件加载到内存里)
            if (!index->BuildIndex(input))
            {
                LOG(FATAL, "构建索引失败");
                return false;
            }
            // std::cout << "构建索引成功" << std::endl;
            LOG(NORMAL, "构建索引成功");
            return true;
        }

        // query:用户的搜索需求 json_string:返回的搜索结果
        void Search(
            const std::string &query,
            std::string *json_string,
            SearchMetadata *metadata = nullptr)
        {
            if (!json_string)
            {
                return;
            }

            std::string cached_result;
            const ns_redis::CacheLookupStatus lookup_status =
                search_cache->LookupResult(query, &cached_result);
            search_cache->RecordKeyword(query);

            if (lookup_status == ns_redis::CacheLookupStatus::Hit)
            {
                if (metadata)
                {
                    metadata->cache_status = SearchCacheStatus::Hit;
                }
                *json_string = cached_result;
                LOG(NORMAL, std::string("search cache HIT: ") + query);
                return;
            }

            if (metadata)
            {
                metadata->cache_status =
                    lookup_status == ns_redis::CacheLookupStatus::Miss
                        ? SearchCacheStatus::Miss
                        : SearchCacheStatus::Bypass;
            }
            LOG(
                NORMAL,
                std::string("search cache ") +
                    (lookup_status == ns_redis::CacheLookupStatus::Miss ? "MISS: " : "BYPASS: ") +
                    query);

            // 1.【分词】:对query进行分词
            std::vector<std::string> words;
            ns_util::JiebaUtil::CutString(query, &words);

            // 2.【触发】:根据query分出来的词，进行index查找
            // ns_index::InvertedList inverted_list_all;

            std::vector<InvertedElemPrint> inverted_list_all;
            // 对不同单词匹配到同一doc_id去重
            std::unordered_map<u_int64_t, InvertedElemPrint> tokens_map;

            for (auto &word : words)
            {
                boost::to_lower(word); // 先将分词转换为小写，因为存储的就是小写

                ns_index::InvertedList *inverted_list = index->GetInvertedList(word);
                if (nullptr == inverted_list)
                {
                    continue;
                }
                // inverted_list_all.insert(inverted_list_all.end(), inverted_list->begin(), inverted_list->end());

                for (const auto &elem : *inverted_list)
                {
                    auto &item = tokens_map[elem.doc_id];
                    item.doc_id = elem.doc_id;
                    item.weight += elem.weight;
                    item.words.push_back(elem.word);
                }
            }

            for (const auto &item : tokens_map)
            {
                inverted_list_all.push_back(std::move(item.second));
            }

            // 3.【合并排序】:汇总查找结果，对结果按weight（权重）进行降序排序
            if (inverted_list_all.empty())
            {
                std::cout << query << " 没有搜索结果" << std::endl;
                *json_string = "[]";
                search_cache->StoreResult(query, *json_string);
                return;
            }
            // std::sort(inverted_list_all.begin(), inverted_list_all.end(),
            //           [](const ns_index::InvertedElem &e1, const ns_index::InvertedElem &e2)
            //           {
            //               return e1.weight > e2.weight;
            //           });
            std::sort(inverted_list_all.begin(), inverted_list_all.end(),
                      [](const InvertedElemPrint &e1, const InvertedElemPrint &e2)
                      {
                          return e1.weight > e2.weight;
                      });
            // 4.【构建】:排序后的结果构建json串
            Json::Value root(Json::arrayValue);
            for (auto &item : inverted_list_all)
            {
                ns_index::DocInfo *doc = index->GetForwardIndex(item.doc_id);
                if (nullptr == doc)
                {
                    continue;
                }
                Json::Value elem;
                elem["title"] = doc->title;
                elem["desc"] = GetDesc(doc->content, item.words[0]);
                elem["url"] = doc->url;
                elem["weight"] = item.weight;
                elem["id"] = doc->doc_id;

                root.append(elem);
            }
            // 将root序列化
            // Json::StyledWriter writer;
            Json::FastWriter writer;
            *json_string = writer.write(root);
            search_cache->StoreResult(query, *json_string);
        }

        bool GetHotKeywords(
            std::size_t limit,
            std::vector<std::pair<std::string, double> > *keywords)
        {
            return search_cache->GetHotKeywords(limit, keywords);
        }

        std::string GetDesc(const std::string &html_content, const std::string &word)
        {
            const int prev_step = 50;
            const int next_step = 100;

            auto iter = std::search(html_content.begin(), html_content.end(), word.begin(), word.end(), [](int x, int y)
                                    { return std::tolower(x) == std::tolower(y); });

            if (iter == html_content.end())
            {
                return "None1";
            }

            int pos = std::distance(html_content.begin(), iter);

            int start = 0;
            int end = html_content.size() - 1;
            if (pos > start + prev_step)
                start = pos - prev_step;
            if (pos + next_step < end)
                end = pos + next_step;

            if (start >= end)
                return "None2";

            return html_content.substr(start, end - start) + "...";
        }
    };
}
