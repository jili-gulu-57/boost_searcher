#include <iostream>
#include <string>
#include <vector>
#include <boost/filesystem.hpp>
#include "util.hpp"

// 一.数据获取与清洗（正排索引）

const std::string src_path = "data/input/"; // 存放所有HTML网页的路径
const std::string output = "data/raw_html/raw.txt";
static int count = 1;

// 存储每个清洗完的文档信息
typedef struct DocInfo
{
    std::string title;   // 文档标题
    std::string content; // 文档内容
    std::string url;
} DocInfo_t;

// const &:输入参数  *:输出参数  &:输入输出

bool EnumFile(const std::string &src_path, std::vector<std::string> *files_list);
bool ParseHtml(const std::vector<std::string> &files_list, std::vector<DocInfo_t> *results);
bool SaveHtml(const std::vector<DocInfo_t> &results, const std::string &output);

int main()
{
    std::vector<std::string> files_list;
    // 1.文件遍历，把每个html文件名带路径保存到files_list中，方便后续查找(数据获取)
    if (!EnumFile(src_path, &files_list))
    {
        std::cerr << "enum file name error" << std::endl;
        return 1;
    }

    // 2.提取文件标题和正文内容(数据清洗)
    std::vector<DocInfo_t> results;
    if (!ParseHtml(files_list, &results))
    {
        std::cerr << "parser html error" << std::endl;
        return 2;
    }

    // 3.解析完后的内容保存到output文件里，以\3为分隔符
    if (!SaveHtml(results, output))
    {
        std::cerr << "save html error" << std::endl;
        return 3;
    }

    return 0;
}

bool EnumFile(const std::string &src_path, std::vector<std::string> *files_list)
{
    namespace fs = boost::filesystem;
    fs::path root_path(src_path);

    if (!fs::exists(root_path)) // 路径不存在
    {
        std::cerr << src_path << " not exists" << std::endl;
        return false;
    }

    fs::recursive_directory_iterator end; // 定义一个空的迭代器
    for (fs::recursive_directory_iterator iter(root_path); iter != end; iter++)
    {
        if (!fs::is_regular_file(*iter)) // 不是一个普通文件
            continue;
        if (iter->path().extension() != ".html") // 不是以.html为结尾
            continue;

        // 合法文件（以.html结尾的网页文件）
        files_list->push_back(iter->path().string());
        // std::cout << "已添加：" << iter->path().string() << std::endl;
    }
    std::cout << "所有网页文件已添加" << std::endl;

    return true;
}

static bool ParseTitle(const std::string &file, std::string *title)
{
    // 找到title出现的起始位置
    std::size_t begin = file.find("<title>");
    if (begin == file.npos)
        return false;
    begin += std::string("<title>").size();

    // 找到title出现的最后位置
    std::size_t end = file.find("</title>");
    if (end == file.npos)
        return false;

    if (begin > end)
        return false;

    if (count)
    {
        std::cout << "title起始位置：" << begin << " 结束位置：" << end << std::endl;
        count--;
    }
    *title = file.substr(begin, end - begin); // 起始位置和步长

    return true;
}

static bool ParseContent(const std::string &file, std::string *content)
{
    // 将标签全部去除，剩下的就是正文内容
    enum status // 简易状态机
    {
        LABLE,
        CONTENT
    };

    enum status s = LABLE; // 起始处于标签状态
    for (char c : file)
    {
        switch (s)
        {
        case LABLE:
            if (c == '>')
                s = CONTENT; // 退出标签状态
            break;

        case CONTENT:
            if (c == '<')
                s = LABLE; // 退出正文状态
            else
            {
                if (c == '\n')
                    c = ' ';

                content->push_back(c);
            }
            break;

        default:
            break;
        }
    }

    return true;
}

static bool ParseUrl(const std::string &file_path, std::string *url)
{
    std::string url_head = "https://www.boost.org/doc/libs/1_90_0/doc/html/";
    std::string url_tail = file_path.substr(src_path.size());
    *url = url_head + url_tail;
    return true;
}

static void ShowDoc(DocInfo_t &doc)
{
    std::cout << "标题为: " << doc.title << std::endl;
    std::cout << "正文为: " << doc.content << std::endl;
    std::cout << "url为: " << doc.url << std::endl;
}

bool ParseHtml(const std::vector<std::string> &files_list, std::vector<DocInfo_t> *results)
{
    std::cout << "进入网页提取" << std::endl;
    for (const std::string &file : files_list)
    {
        // ①从路径里将文件内容读取出来
        std::string result;
        if (!ns_util::FileUtil::ReadFile(file, &result))
        {
            std::cout << "文件内容提取失败" << std::endl;
            continue;
        }

        DocInfo_t doc;
        // ②从文件内容提取title
        if (!ParseTitle(result, &doc.title))
        {
            std::cout << "文件title提取失败" << std::endl;
            continue;
        }

        // ③从文件内容提取正文
        if (!ParseContent(result, &doc.content))
        {
            std::cout << "文件content提取失败" << std::endl;
            continue;
        }

        // ④构建文件的url
        if (!ParseUrl(file, &doc.url))
        {
            std::cout << "文件url提取失败" << std::endl;

            continue;
        }

        results->push_back(std::move(doc));
    }
    // debug:
    // ShowDoc(results->at(results->size() - 1));
    return true;
}

bool SaveHtml(const std::vector<DocInfo_t> &results, const std::string &output)
{
#define SEP '\3' // 文档内分隔符
    // 按照二进制方式进行写入
    std::ofstream out(output, std::ios::out | std::ios::binary); // 打开output文件，以二进制格式写入
    if (!out.is_open())
    {
        std::cerr << "open " << output << " fail" << std::endl;
        return false;
    }

    for (auto &item : results)
    {
        std::string out_string;
        out_string = item.title;
        out_string += SEP;
        out_string += item.content;
        out_string += SEP;
        out_string += item.url;
        out_string += '\n';

        out.write(out_string.c_str(), out_string.size());
    }

    out.close();
    return true;
}
