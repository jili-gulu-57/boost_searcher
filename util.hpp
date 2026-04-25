#pragma once
// 工具类

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <boost/algorithm/string.hpp>
#include <mutex>
#include "jieba/Jieba.hpp"
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

    const char *const DICT_PATH = "./dict/jieba.dict.utf8";
    const char *const HMM_PATH = "./dict/hmm_model.utf8";
    const char *const USER_DICT_PATH = "./dict/user.dict.utf8";
    const char *const IDF_PATH = "./dict/idf.utf8";
    const char *const STOP_WORD_PATH = "./dict/stop_words.utf8";

    class JiebaUtil
    {
    private:
        // static cppjieba::Jieba jieba;
        cppjieba::Jieba jieba;
        // std::unordered_map<std::string, bool> stop_words;
        std::unordered_set<std::string> stop_words;

    private:
        JiebaUtil() : jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH, IDF_PATH, STOP_WORD_PATH)
        {
        }
        JiebaUtil(const JiebaUtil &) = delete;
        JiebaUtil &operator=(const JiebaUtil &) = delete;

        static JiebaUtil *instance;

    public:
        static JiebaUtil *get_instance()
        {
            static std::mutex mtx;
            if (nullptr == instance)
            {
                mtx.lock();
                if (nullptr == instance)
                {
                    instance = new JiebaUtil();
                    instance->InitJiebaUtil();
                }
                mtx.unlock();
            }
            return instance;
        }

        void InitJiebaUtil()
        {
            std::ifstream in(STOP_WORD_PATH);
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
    // cppjieba::Jieba JiebaUtil::jieba(DICT_PATH,HMM_PATH,USER_DICT_PATH,IDF_PATH,STOP_WORD_PATH);
    JiebaUtil *JiebaUtil::instance = nullptr;

} // namespace ns_util
