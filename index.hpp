#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <mutex>
#include "util.hpp"
#include "log.hpp"

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

    // 倒排拉链,存储某一关键字的所有信息
    typedef std::vector<InvertedElem> InvertedList;
    class Index
    {
    private:
        // 构建正排索引  key值：下标（doc_id）,value值：DocInfo对象
        std::vector<DocInfo> forward_index;
        // 构建倒排索引  key值：关键字，value值：包含该关键字的所有文档
        std::unordered_map<std::string, InvertedList> inverted_index;

    private:
        Index() {}                                // 设为私有，禁止外部直接创建对象
        Index(const Index &) = delete;            // 把拷贝构造函数显示删除，禁止通过克隆创建新对象
        Index &operator=(const Index &) = delete; // 把赋值重载运算符显示删除，禁止通过赋值产生副本

        static Index *instance; // 单例化Index对象，实现节省空间和全局唯一
        static std::mutex mtx;

    public:
        ~Index()
        {
        }

    public:
        static Index *GetInstance()
        {
            // 双重加锁，保证对象创建安全
            if (nullptr == instance)
            {
                mtx.lock();
                if (nullptr == instance)
                {
                    instance = new Index();
                }
                mtx.unlock();
            }

            return instance;
        }
        DocInfo *GetForwardIndex(u_int64_t doc_id)
        {
            if (doc_id >= forward_index.size())
            {
                std::cerr << "doc_id out of range" << std::endl;
                return nullptr;
            }
            return &forward_index[doc_id];
        }

        InvertedList *GetInvertedList(const std::string &word)
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
            static int count = 0;
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

                count++;
                if (count % 100 == 0)
                {
                    // std::cout << "索引构建成功：" << count << std::endl;
                    std::string msg = "索引构建成功：" + std::to_string(count);
                    LOG(NORMAL, msg);
                }
            }
            return true;
        }

    private:
        DocInfo *BuildForwardIndex(const std::string &line)
        {
            // 1.解析字符串：title，content，url
            std::vector<std::string> results;
            const std::string SEP = "\3";
            ns_util::StringUtil::Split(line, &results, SEP);
            if (results.size() != 3)
            {
                return nullptr;
            }

            // 2.解析结果填充到DocInfo中
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
            // 分词，做词频处理
            struct word_cnt
            {
                int title_cnt;
                int content_cnt;

                word_cnt() : title_cnt(0), content_cnt(0) {}
            };

            // 统计所有分词在title和content出现的次数
            std::unordered_map<std::string, word_cnt> word_map;
            std::vector<std::string> title_words;
            ns_util::JiebaUtil::CutString(doc.title, &title_words);
            for (auto s : title_words)
            {
                // 不区分大小写
                boost::to_lower(s);
                word_map[s].title_cnt++;
            }

            std::vector<std::string> content_words;
            ns_util::JiebaUtil::CutString(doc.content, &content_words);
            for (auto s : content_words)
            {
                boost::to_lower(s);
                word_map[s].content_cnt++;
            }
#define X 10
#define Y 1
            for (auto &word_pair : word_map)
            {
                InvertedElem item;
                item.doc_id = doc.doc_id;
                item.word = word_pair.first;
                item.weight = X * word_pair.second.title_cnt + Y * word_pair.second.content_cnt;
                InvertedList &inverted_list = inverted_index[word_pair.first];
                inverted_list.push_back(std::move(item));
            }
            return true;
        }
    };
    Index *Index::instance = nullptr;
    std::mutex Index::mtx;
}