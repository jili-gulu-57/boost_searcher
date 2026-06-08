# Boost Searcher：C++ 本地搜索引擎

基于 C++14、Boost.Beast、cppjieba、正排索引和倒排索引实现的本地全文搜索服务。项目通过 HTTP 返回 JSON 搜索结果，并使用 Redis 缓存热点查询、统计热门搜索词。Redis 不可用时自动降级到原有本地倒排索引检索，不影响基础搜索能力。

## 核心链路

```text
用户请求
  -> Redis 热点词计数 ZINCRBY
  -> 查询 search:result:{keyword}
  -> cache hit：直接返回 JSON
  -> cache miss：本地分词与倒排索引检索
  -> 搜索结果写入 Redis，设置 TTL
  -> HTTP 返回结果和 X-Cache 响应头
```

## 功能

- cppjieba 关键词分词和停用词过滤。
- 正排索引与倒排索引构建。
- 多关键词权重聚合和搜索结果排序。
- Boost.Beast 多线程 HTTP 服务。
- Redis 热点搜索结果缓存。
- Redis ZSet 搜索词热度统计。
- Redis 异常、超时或编译时缺失 hiredis 时自动降级。
- 通过环境变量配置远程 Redis IP、端口、密码、数据库和 TTL。

## Redis 安装与启动

Ubuntu/Debian 安装 Redis 和 hiredis：

```bash
sudo apt update
sudo apt install redis-server libhiredis-dev
sudo systemctl enable --now redis-server
redis-cli ping
```

正常返回：

```text
PONG
```

如果 Redis 运行在远程 Linux 服务器，需要确认：

- Redis 配置允许远程连接。
- 防火墙只向可信客户端开放 Redis 端口。
- 建议设置密码并限制绑定地址，不要将无密码 Redis 暴露到公网。

## Redis 配置

默认连接 `127.0.0.1:6379`，数据库为 `0`，缓存 TTL 为 `300` 秒。

```bash
export REDIS_ENABLED=true
export REDIS_HOST=192.168.1.100
export REDIS_PORT=6379
export REDIS_PASSWORD='your-password'
export REDIS_DB=0
export REDIS_CACHE_TTL=300
export REDIS_CONNECT_TIMEOUT_MS=300
export REDIS_SOCKET_TIMEOUT_MS=500
```

可参考 `redis.env.example`。远程服务器只需要修改 `REDIS_HOST` 和 `REDIS_PORT`，有认证时再设置 `REDIS_PASSWORD`。

禁用 Redis、强制使用本地检索：

```bash
export REDIS_ENABLED=false
```

## 本地编译

安装构建依赖：

```bash
sudo apt install build-essential cmake pkg-config \
  libboost-filesystem-dev libboost-system-dev libjsoncpp-dev libhiredis-dev
```

CMake 构建：

```bash
cmake -S . -B build -DCPPJIEBA_ROOT=/path/to/cppjieba
cmake --build build --parallel
```

如果 hiredis 存在，CMake 会显示：

```text
Redis support enabled with hiredis
```

如果 hiredis 不存在，项目仍可编译，但 Redis 功能会关闭，本地检索保持可用。

也可使用 Makefile：

```bash
make
```

## 运行

```bash
export SEARCHER_DICT_DIR=/path/to/cppjieba/dict
export REDIS_HOST=你的远程Redis地址
export REDIS_PORT=6379
export REDIS_PASSWORD='你的密码'

./build/search_server 8080 4
```

参数 `8080` 是 HTTP 端口，`4` 是工作线程数。

## HTTP 接口

| 方法 | 路径 | 说明 |
| --- | --- | --- |
| `GET` | `/health` | 服务健康检查 |
| `GET` | `/search?query=boost` | 搜索并返回 JSON 数组 |
| `GET` | `/hot_keywords?limit=10` | 返回 Top N 热门搜索词 |

搜索接口响应头包含缓存状态：

- `X-Cache: MISS`：Redis 可用但没有缓存，执行了本地倒排索引检索。
- `X-Cache: HIT`：直接返回 Redis 中的搜索结果。
- `X-Cache: BYPASS`：Redis 不可用、未安装或被禁用，已降级到本地检索。

## 验证 Redis 是否生效

第一次查询：

```bash
curl -i "http://127.0.0.1:8080/search?query=boost"
```

应看到：

```text
X-Cache: MISS
```

再次执行相同查询：

```bash
curl -i "http://127.0.0.1:8080/search?query=boost"
```

应看到：

```text
X-Cache: HIT
```

查看 Redis 中的缓存和 TTL：

```bash
redis-cli -h "$REDIS_HOST" -p "$REDIS_PORT" -a "$REDIS_PASSWORD" \
  GET "search:result:boost"
redis-cli -h "$REDIS_HOST" -p "$REDIS_PORT" -a "$REDIS_PASSWORD" \
  TTL "search:result:boost"
```

查看热门词：

```bash
curl "http://127.0.0.1:8080/hot_keywords?limit=10"

redis-cli -h "$REDIS_HOST" -p "$REDIS_PORT" -a "$REDIS_PASSWORD" \
  ZREVRANGE search:hot_keywords 0 9 WITHSCORES
```

## 测试与 Demo

独立 Redis 缓存 demo：

```bash
./build/redis_cache_demo
```

Redis 可用时会输出：

```text
first query: cache MISS
second query: cache HIT
hot keywords:
```

Redis 不可用时会输出 BYPASS，并正常退出，不会导致程序崩溃。

HTTP smoke test：

```bash
bash scripts/smoke_test.sh
```

## Docker

```bash
docker build -t boost-searcher .
docker run --rm -p 8080:8080 \
  -e REDIS_HOST=192.168.1.100 \
  -e REDIS_PORT=6379 \
  -e REDIS_PASSWORD='your-password' \
  boost-searcher
```

容器访问宿主机 Redis 时，可根据系统使用宿主机局域网 IP 或 `host.docker.internal`。

## Redis 优化实现

- `RedisClient`：封装 hiredis 连接、认证、数据库选择、超时和自动重连。
- `SearchCache`：封装搜索结果缓存 key、TTL、热度计数和 Top N 查询。
- Redis context 使用互斥锁保护，适配 HTTP 工作线程并发访问。
- 连接或命令失败后释放异常连接，并在短暂间隔后重试。
- 缓存 key 格式：`search:result:{keyword}`。
- 热门词 ZSet key：`search:hot_keywords`。

## 项目优化点

- 引入 Redis 缓存热点搜索结果，减少重复倒排索引检索开销。
- 基于 Redis ZSet 实现搜索词热度统计，支持热门关键词排行。
- Redis 异常时支持降级到本地检索，提升系统可用性。
- 通过 TTL 控制缓存生命周期，避免缓存长期占用内存。
