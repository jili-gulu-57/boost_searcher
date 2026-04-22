#pragma once

#include "index.hpp"
#include "util.hpp"
#include <algorithm>
#include <jsoncpp/json/json.h>

namespace ns_searcher
{
    class Searcher
    {
    private:
        ns_index::Index *index;

    public:
        void InitSearcher(const std::string &input)
        {
            // 1.创建index对象
            index = ns_index::Index::GetInstance();
            std::cout << "建立Index对象成功" << std::endl;
            // 2.构建索引(将硬盘里的索引文件加载到内存里)
            index->BuildIndex(input);
            std::cout << "构建索引成功" << std::endl;
        }

        // query:用户的搜索需求 json_string:返回的搜索结果
        void Search(const std::string &query, std::string *json_string)
        {
            // 1.【分词】:对query进行分词
            std::vector<std::string> words;
            ns_util::JiebaUtil::CutString(query, &words);

            // 2.【触发】:根据query分出来的词，进行index查找
            ns_index::InvertedList inverted_list_all;
            for (auto &word : words)
            {
                boost::to_lower(word); // 先将分词转换为小写，因为存储的就是小写

                ns_index::InvertedList *inverted_list = index->GetInvertedList(word);
                if (nullptr == inverted_list)
                {
                    continue;
                }
                inverted_list_all.insert(inverted_list_all.end(), inverted_list->begin(), inverted_list->end());
            }

            // 3.【合并排序】:汇总查找结果，对结果按weight（权重）进行降序排序
            if (inverted_list_all.empty())
            {
                std::cout << query << " 没有搜索结果" << std::endl;
                json_string->clear();
                return;
            }
            std::sort(inverted_list_all.begin(), inverted_list_all.end(),
                      [](const ns_index::InvertedElem &e1, const ns_index::InvertedElem &e2)
                      {
                          return e1.weight > e2.weight;
                      });
            // 4.【构建】:排序后的结果构建json串
            Json::Value root;
            for (auto &item : inverted_list_all)
            {
                ns_index::DocInfo *doc = index->GetForwardIndex(item.doc_id);
                if (nullptr == doc)
                {
                    continue;
                }
                Json::Value elem;
                elem["title"] = doc->title;
                elem["desc"] = GetDesc(doc->content, item.word);
                elem["url"] = doc->url;
                elem["weight"] = item.weight;

                root.append(elem);
            }
            // 将root序列化
            Json::StyledWriter writer;
            *json_string = writer.write(root);
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

            return html_content.substr(start, end - start);
        }
    };
}