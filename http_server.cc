#include "searcher.hpp"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <jsoncpp/json/json.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = asio::ip::tcp;

namespace
{
const std::string kInput = "data/raw_html/raw.txt";

std::string UrlDecode(const std::string &value)
{
    std::string decoded;
    decoded.reserve(value.size());
    for (std::size_t index = 0; index < value.size(); ++index)
    {
        if (value[index] == '+')
        {
            decoded.push_back(' ');
            continue;
        }
        if (value[index] == '%' && index + 2 < value.size())
        {
            const std::string hex = value.substr(index + 1, 2);
            char *end = nullptr;
            const long code = std::strtol(hex.c_str(), &end, 16);
            if (end == hex.c_str() + 2)
            {
                decoded.push_back(static_cast<char>(code));
                index += 2;
                continue;
            }
        }
        decoded.push_back(value[index]);
    }
    return decoded;
}

std::string QueryParam(const std::string &target, const std::string &name)
{
    const auto query_pos = target.find('?');
    if (query_pos == std::string::npos)
    {
        return "";
    }

    std::size_t begin = query_pos + 1;
    while (begin < target.size())
    {
        const auto end = target.find('&', begin);
        const std::string item = target.substr(begin, end - begin);
        const auto separator = item.find('=');
        if (separator != std::string::npos && item.substr(0, separator) == name)
        {
            return UrlDecode(item.substr(separator + 1));
        }
        if (end == std::string::npos)
        {
            break;
        }
        begin = end + 1;
    }
    return "";
}

std::size_t QueryLimit(const std::string &target)
{
    const std::string value = QueryParam(target, "limit");
    if (value.empty())
    {
        return 10;
    }
    try
    {
        return std::min<std::size_t>(100, std::max<std::size_t>(1, std::stoul(value)));
    }
    catch (...)
    {
        return 10;
    }
}

http::response<http::string_body> JsonResponse(
    http::status status,
    const std::string &body,
    unsigned version)
{
    http::response<http::string_body> response{status, version};
    response.set(http::field::server, "boost-searcher");
    response.set(http::field::content_type, "application/json; charset=utf-8");
    response.keep_alive(false);
    response.body() = body;
    response.prepare_payload();
    return response;
}

http::response<http::string_body> HandleRequest(
    const http::request<http::string_body> &request,
    ns_searcher::Searcher &searcher)
{
    if (request.method() != http::verb::get)
    {
        return JsonResponse(http::status::method_not_allowed, R"({"error":"only GET is supported"})", request.version());
    }

    const std::string target(request.target().data(), request.target().size());
    const std::string path = target.substr(0, target.find('?'));
    if (path == "/health")
    {
        return JsonResponse(http::status::ok, R"({"status":"ok"})", request.version());
    }
    if (path == "/hot_keywords")
    {
        std::vector<std::pair<std::string, double> > keywords;
        if (!searcher.GetHotKeywords(QueryLimit(target), &keywords))
        {
            return JsonResponse(
                http::status::service_unavailable,
                R"({"error":"redis is unavailable"})",
                request.version());
        }

        Json::Value root(Json::arrayValue);
        for (const auto &item : keywords)
        {
            Json::Value keyword;
            keyword["keyword"] = item.first;
            keyword["score"] = item.second;
            root.append(keyword);
        }
        Json::FastWriter writer;
        return JsonResponse(http::status::ok, writer.write(root), request.version());
    }
    if (path != "/search")
    {
        return JsonResponse(http::status::not_found, R"({"error":"route not found"})", request.version());
    }

    const std::string query = QueryParam(target, "query");
    if (query.empty())
    {
        return JsonResponse(http::status::bad_request, R"({"error":"query parameter is required"})", request.version());
    }

    std::string results;
    ns_searcher::SearchMetadata metadata;
    searcher.Search(query, &results, &metadata);
    auto response = JsonResponse(http::status::ok, results, request.version());
    response.set("X-Cache", ns_searcher::SearchCacheStatusName(metadata.cache_status));
    return response;
}

void Serve(tcp::socket socket, ns_searcher::Searcher &searcher)
{
    try
    {
        beast::flat_buffer buffer;
        http::request<http::string_body> request;
        http::read(socket, buffer, request);
        auto response = HandleRequest(request, searcher);
        http::write(socket, response);
        socket.shutdown(tcp::socket::shutdown_send);
    }
    catch (const std::exception &error)
    {
        LOG(WARNING, std::string("request failed: ") + error.what());
    }
}
} // namespace

int main(int argc, char *argv[])
{
    const unsigned short port = argc > 1 ? static_cast<unsigned short>(std::stoi(argv[1])) : 8080;
    const std::size_t worker_count = argc > 2 ? std::stoul(argv[2]) : std::max(2u, std::thread::hardware_concurrency());

    ns_searcher::Searcher searcher;
    if (!searcher.InitSearcher(kInput))
    {
        return 1;
    }

    asio::io_context context;
    tcp::acceptor acceptor(context, {tcp::v4(), port});
    asio::thread_pool workers(worker_count);
    LOG(NORMAL, "HTTP server listening on 0.0.0.0:" + std::to_string(port));

    while (true)
    {
        tcp::socket socket(context);
        acceptor.accept(socket);
        asio::post(workers, [socket = std::move(socket), &searcher]() mutable
                   { Serve(std::move(socket), searcher); });
    }
}
