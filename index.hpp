#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include "util.hpp"

namespace ns_index
{
    // 正排索引节点
    struct DocInfo
    {
        std::string title;
        std::string content;
        std::string url;
        u_int64_t doc_id;
    };

    // 倒排索引节点
    struct InvertedElem
    {
        u_int64_t doc_id;
        std::string word; // 关键字
        int weight;       // 关键字在文档内的权重
    };

    typedef std::vector<InvertedElem> InvertedList; // 倒排拉链
    class Index
    {
    private:
        // 构建正排索引  key值：下标（doc_id）,value值：DocInfo对象
        std::vector<DocInfo> forward_index;
        // 构建倒排索引  key值：关键字，value值：包含该关键字的所有文档
        std::unordered_map<std::string, InvertedList> inverted_index;

    public:
        Index()
        {
        }
        ~Index()
        {
        }

    public:
        DocInfo *GetForwardIndex(u_int64_t doc_id)
        {
            if (doc_id >= forward_index.size())
            {
                std::cerr << "doc_id out of range" << std::endl;
                return nullptr;
            }
            return &forward_index[doc_id];
        }

        InvertedList *GetInvertedIndex(const std::string &word)
        {
            auto iter = inverted_index.find(word);
            if (iter == inverted_index.end())
            {
                std::cerr << word << " 没有匹配结果" << std::endl;
                return nullptr;
            }
            return &(iter->second);
        }

        // 根据清洗后的数据，构建正排倒排索引
        bool BuildIndex(const std::string &input)
        {
            std::ifstream in(input, std::ios::in | std::ios::binary);
            if (!in.is_open())
            {
                std::cerr << input << " open error" << std::endl;
                return false;
            }

            std::string line;
            while (std::getline(in, line))
            {
                DocInfo *doc = BuildForwardIndex(line);
                if (nullptr == doc)
                {
                    std::cerr << "build " << line << " fail" << std::endl;
                    continue;
                }
                BulidInvertedIndex(*doc);
            }
            return true;
        }

    private:
        DocInfo *BuildForwardIndex(const std::string &line)
        {
            //1.解析字符串：title，content，url
            std::vector<std::string> results;
            const std::string SEP = "\3";
            ns_util::StringUtil::CutString(line, &results, SEP);
            if(results.size()!=3)
            {
                return nullptr;
            }

            //2.解析结果填充到DocInfo中
            DocInfo doc;
            doc.title = results[0];
            doc.content = results[1];
            doc.url = results[2];
            doc.doc_id = forward_index.size();
            // 3.插入到正排索引的vector中
            forward_index.push_back(std::move(doc));

            return &forward_index.back();
        }
        bool BulidInvertedIndex(const DocInfo &doc)
        {
            
        }
    };
}