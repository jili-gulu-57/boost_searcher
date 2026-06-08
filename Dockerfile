FROM ubuntu:24.04 AS builder

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential cmake git libboost-filesystem-dev libboost-system-dev \
        libjsoncpp-dev libhiredis-dev pkg-config \
    && rm -rf /var/lib/apt/lists/*

RUN git clone --depth 1 https://github.com/yanyiwu/cppjieba.git /opt/cppjieba

WORKDIR /app
COPY CMakeLists.txt parser.cc server.cc http_server.cc searcher.hpp index.hpp util.hpp log.hpp \
     config.h config.cc redis_client.h redis_client.cc search_cache.h search_cache.cc ./
COPY test/redis_cache_demo.cc ./test/redis_cache_demo.cc
RUN cmake -S . -B build -DCPPJIEBA_ROOT=/opt/cppjieba \
    && cmake --build build --parallel

FROM ubuntu:24.04
RUN apt-get update \
    && apt-get install -y --no-install-recommends libboost-system1.83.0 libjsoncpp25 libhiredis-dev curl \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /app/build/search_server /app/search_server
COPY --from=builder /opt/cppjieba/dict /opt/cppjieba/dict
COPY data/raw_html/raw.txt /app/data/raw_html/raw.txt
COPY scripts/smoke_test.sh /app/scripts/smoke_test.sh

WORKDIR /app
ENV SEARCHER_DICT_DIR=/opt/cppjieba/dict
EXPOSE 8080
ENTRYPOINT ["/app/search_server"]
