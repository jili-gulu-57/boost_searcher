#include "searcher.hpp"
#include "cpp-httplib-0.27.0/httplib.h"

const std::string root_path = "./wwwroot";
const std::string input = "data/raw_html/raw.txt";

int main()
{
    ns_searcher::Searcher search;
    search.InitSearcher(input);

    httplib::Server svr;
    svr.set_base_dir(root_path.c_str());
    svr.Get("/s", [&search](const httplib::Request &req, httplib::Response &rsp)
            { if(!req.has_param("word"))
                {
                    rsp.set_content("必须要有搜索关键字！", "text/plain;charset=utf-8");
                    return;
                }
                std::string word=req.get_param_value("word");
                // std::cout<<"用户在搜索："<<word<<std::endl;
                std::string msg = "用户在搜索：" + word;
                LOG(NORMAL, msg);
                std::string json_string;
                search.Search(word,&json_string);
                rsp.set_content(json_string,"application/json"); });
    svr.listen("0.0.0.0", 8080);
    return 0;
}