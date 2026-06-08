#pragma once
// 工具类

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <boost/algorithm/string.hpp>
#include <cstdlib>
#include <mutex>
#include <cppjieba/Jieba.hpp>
#include "log.hpp"

namespace ns_util
{
    class FileUtil
    {
    public:
        static bool ReadFile(const std::string &file_path, std::string *out)
        {
            // 创建一个输入文件流对象in
            std::ifstream in(file_path, std::ios::in); // std::ios::in 是一个打开模式标志，表示以 读取模式 打开文件。
            if (!in.is_open())
            {
                std::cerr << "open file" << file_path << " error" << std::endl;
                return false;
            }

            std::string line;
            while (std::getline(in, line))
            {
                *out += line;
            }

            in.close();
            return true;
        }
    };

    class StringUtil
    {
    public:
        static void Split(const std::string &target, std::vector<std::string> *out, std::string sep)
        {
            boost::split(*out, target, boost::is_any_of("\3"), boost::token_compress_on);
        }
    };

    inline std::string DictPath(const std::string &filename)
    {
        const char *configured_dir = std::getenv("SEARCHER_DICT_DIR");
        const std::string dict_dir = configured_dir ? configured_dir : "./cppjieba/dict";
        return dict_dir + "/" + filename;
    }

    class JiebaUtil
    {
    private:
        // static cppjieba::Jieba jieba;
        cppjieba::Jieba jieba;
        std::unordered_set<std::string> stop_words;
        std::mutex cut_mutex;

    private:
        JiebaUtil() : jieba(
                          DictPath("jieba.dict.utf8"),
                          DictPath("hmm_model.utf8"),
                          DictPath("user.dict.utf8"),
                          DictPath("idf.utf8"),
                          DictPath("stop_words.utf8"))
        {
        }
        JiebaUtil(const JiebaUtil &) = delete;
        JiebaUtil &operator=(const JiebaUtil &) = delete;

    public:
        static JiebaUtil *get_instance()
        {
            static JiebaUtil instance;
            static const bool initialized = []()
            {
                instance.InitJiebaUtil();
                return true;
            }();
            (void)initialized;
            return &instance;
        }

        void InitJiebaUtil()
        {
            std::ifstream in(DictPath("stop_words.utf8"));
            if (!in.is_open())
            {
                LOG(FATAL, "load stop words file error");
                return;
            }

            std::string line;
            while (std::getline(in, line))
            {
                stop_words.insert(line);
            }
            in.close();
        }

        void CutStringHelper(const std::string &src, std::vector<std::string> *out)
        {
            std::lock_guard<std::mutex> lock(cut_mutex);
            jieba.CutForSearch(src, *out);
            for (auto iter = out->begin(); iter != out->end();)
            {
                auto it = stop_words.find(*iter);
                if (it != stop_words.end())
                {
                    iter = out->erase(iter);
                }
                else
                    iter++;
            }
        }

        static void CutString(const std::string &src, std::vector<std::string> *out)
        {
            ns_util::JiebaUtil::get_instance()->CutStringHelper(src, out);
            // jieba.CutForSearch(src, *out);
        }
    };
} // namespace ns_util
