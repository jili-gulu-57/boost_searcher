#include "redis_client.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <mutex>

#ifdef SEARCHER_ENABLE_REDIS
#include <hiredis/hiredis.h>
#include <sys/time.h>
#endif

namespace ns_redis
{
    class RedisClient::Impl
    {
    public:
        explicit Impl(const RedisConfig &redis_config)
            : config(redis_config),
              last_connect_attempt(),
              has_connect_attempt(false)
#ifdef SEARCHER_ENABLE_REDIS
              ,
              context(nullptr)
#endif
        {
        }

        ~Impl()
        {
#ifdef SEARCHER_ENABLE_REDIS
            std::lock_guard<std::mutex> lock(mutex);
            ResetLocked();
#endif
        }

        RedisConfig config;
        std::mutex mutex;
        std::chrono::steady_clock::time_point last_connect_attempt;
        bool has_connect_attempt;

#ifdef SEARCHER_ENABLE_REDIS
        redisContext *context;

        void ResetLocked()
        {
            if (context)
            {
                redisFree(context);
                context = nullptr;
            }
        }

        bool ReplySucceeded(redisReply *reply)
        {
            return reply && reply->type != REDIS_REPLY_ERROR;
        }

        bool ConnectLocked()
        {
            if (!config.enabled)
            {
                return false;
            }
            if (context && context->err == 0)
            {
                return true;
            }

            const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
            if (has_connect_attempt &&
                std::chrono::duration_cast<std::chrono::seconds>(now - last_connect_attempt).count() < 5)
            {
                return false;
            }
            has_connect_attempt = true;
            last_connect_attempt = now;
            ResetLocked();

            timeval connect_timeout;
            connect_timeout.tv_sec = config.connect_timeout_ms / 1000;
            connect_timeout.tv_usec = (config.connect_timeout_ms % 1000) * 1000;
            context = redisConnectWithTimeout(config.host.c_str(), config.port, connect_timeout);
            if (!context || context->err)
            {
                std::cerr << "[Redis] connection failed, fallback to local search: "
                          << (context ? context->errstr : "cannot allocate redis context") << std::endl;
                ResetLocked();
                return false;
            }

            timeval socket_timeout;
            socket_timeout.tv_sec = config.socket_timeout_ms / 1000;
            socket_timeout.tv_usec = (config.socket_timeout_ms % 1000) * 1000;
            redisSetTimeout(context, socket_timeout);

            if (!config.password.empty())
            {
                redisReply *reply = static_cast<redisReply *>(
                    redisCommand(context, "AUTH %b", config.password.data(), config.password.size()));
                const bool ok = ReplySucceeded(reply);
                if (reply)
                {
                    freeReplyObject(reply);
                }
                if (!ok)
                {
                    std::cerr << "[Redis] authentication failed, fallback to local search" << std::endl;
                    ResetLocked();
                    return false;
                }
            }

            redisReply *select_reply = static_cast<redisReply *>(
                redisCommand(context, "SELECT %d", config.db));
            const bool selected = ReplySucceeded(select_reply);
            if (select_reply)
            {
                freeReplyObject(select_reply);
            }
            if (!selected)
            {
                std::cerr << "[Redis] SELECT failed, fallback to local search" << std::endl;
                ResetLocked();
                return false;
            }

            std::cout << "[Redis] connected to " << config.host << ":" << config.port
                      << " db=" << config.db << std::endl;
            return true;
        }

        void HandleCommandFailureLocked()
        {
            std::cerr << "[Redis] command failed, fallback to local search" << std::endl;
            ResetLocked();
        }
#endif
    };

    RedisClient::RedisClient(const RedisConfig &config)
        : impl_(new Impl(config))
    {
#ifndef SEARCHER_ENABLE_REDIS
        if (config.enabled)
        {
            std::cerr << "[Redis] hiredis was not found at build time; Redis features are disabled" << std::endl;
        }
#endif
    }

    RedisClient::~RedisClient()
    {
    }

    RedisGetStatus RedisClient::Get(const std::string &key, std::string *value)
    {
        if (!value)
        {
            return RedisGetStatus::Error;
        }
#ifdef SEARCHER_ENABLE_REDIS
        std::lock_guard<std::mutex> lock(impl_->mutex);
        if (!impl_->ConnectLocked())
        {
            return RedisGetStatus::Error;
        }

        redisReply *reply = static_cast<redisReply *>(
            redisCommand(impl_->context, "GET %b", key.data(), key.size()));
        if (!reply)
        {
            impl_->HandleCommandFailureLocked();
            return RedisGetStatus::Error;
        }

        RedisGetStatus status = RedisGetStatus::Error;
        if (reply->type == REDIS_REPLY_STRING)
        {
            value->assign(reply->str, reply->len);
            status = RedisGetStatus::Hit;
        }
        else if (reply->type == REDIS_REPLY_NIL)
        {
            status = RedisGetStatus::Miss;
        }
        else if (reply->type == REDIS_REPLY_ERROR)
        {
            impl_->HandleCommandFailureLocked();
        }
        freeReplyObject(reply);
        return status;
#else
        (void)key;
        return RedisGetStatus::Error;
#endif
    }

    bool RedisClient::SetEx(const std::string &key, int ttl_seconds, const std::string &value)
    {
#ifdef SEARCHER_ENABLE_REDIS
        std::lock_guard<std::mutex> lock(impl_->mutex);
        if (!impl_->ConnectLocked())
        {
            return false;
        }
        redisReply *reply = static_cast<redisReply *>(
            redisCommand(
                impl_->context,
                "SETEX %b %d %b",
                key.data(),
                key.size(),
                ttl_seconds,
                value.data(),
                value.size()));
        const bool ok = impl_->ReplySucceeded(reply);
        if (reply)
        {
            freeReplyObject(reply);
        }
        if (!ok)
        {
            impl_->HandleCommandFailureLocked();
        }
        return ok;
#else
        (void)key;
        (void)ttl_seconds;
        (void)value;
        return false;
#endif
    }

    bool RedisClient::ZIncrBy(const std::string &key, const std::string &member)
    {
#ifdef SEARCHER_ENABLE_REDIS
        std::lock_guard<std::mutex> lock(impl_->mutex);
        if (!impl_->ConnectLocked())
        {
            return false;
        }
        redisReply *reply = static_cast<redisReply *>(
            redisCommand(
                impl_->context,
                "ZINCRBY %b 1 %b",
                key.data(),
                key.size(),
                member.data(),
                member.size()));
        const bool ok = impl_->ReplySucceeded(reply);
        if (reply)
        {
            freeReplyObject(reply);
        }
        if (!ok)
        {
            impl_->HandleCommandFailureLocked();
        }
        return ok;
#else
        (void)key;
        (void)member;
        return false;
#endif
    }

    bool RedisClient::ZRevRangeWithScores(
        const std::string &key,
        std::size_t limit,
        std::vector<std::pair<std::string, double> > *items)
    {
        if (!items)
        {
            return false;
        }
        items->clear();
        if (limit == 0)
        {
            return true;
        }
#ifdef SEARCHER_ENABLE_REDIS
        std::lock_guard<std::mutex> lock(impl_->mutex);
        if (!impl_->ConnectLocked())
        {
            return false;
        }

        const std::string end_index = std::to_string(limit - 1);
        redisReply *reply = static_cast<redisReply *>(
            redisCommand(
                impl_->context,
                "ZREVRANGE %b 0 %b WITHSCORES",
                key.data(),
                key.size(),
                end_index.data(),
                end_index.size()));
        if (!reply || reply->type != REDIS_REPLY_ARRAY)
        {
            if (reply)
            {
                freeReplyObject(reply);
            }
            impl_->HandleCommandFailureLocked();
            return false;
        }

        for (std::size_t index = 0; index + 1 < reply->elements; index += 2)
        {
            redisReply *member = reply->element[index];
            redisReply *score = reply->element[index + 1];
            if (!member || !score || !member->str || !score->str)
            {
                continue;
            }
            items->push_back(std::make_pair(
                std::string(member->str, member->len),
                std::strtod(score->str, nullptr)));
        }
        freeReplyObject(reply);
        return true;
#else
        (void)key;
        return false;
#endif
    }

    bool RedisClient::IsEnabled() const
    {
        return impl_->config.enabled;
    }
} // namespace ns_redis
