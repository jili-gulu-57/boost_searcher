CXX ?= g++
CXXFLAGS ?= -std=c++14 -O2
CPPJIEBA_CFLAGS ?= -I./cppjieba/include
JSONCPP_CFLAGS := $(shell pkg-config --cflags jsoncpp 2>/dev/null)
JSONCPP_LIBS := $(shell pkg-config --libs jsoncpp 2>/dev/null)

ifeq ($(strip $(JSONCPP_LIBS)),)
JSONCPP_LIBS = -ljsoncpp
endif

PARSER = parser
DEBUG = debug
HTTP_SERVER = http_server
REDIS_DEMO = redis_cache_demo
REDIS_SOURCES = config.cc redis_client.cc search_cache.cc

HIREDIS_CFLAGS := $(shell pkg-config --cflags hiredis 2>/dev/null)
HIREDIS_LIBS := $(shell pkg-config --libs hiredis 2>/dev/null)

ifneq ($(strip $(HIREDIS_LIBS)),)
REDIS_FLAGS = -DSEARCHER_ENABLE_REDIS $(HIREDIS_CFLAGS)
endif

.PHONY: all clean
all: $(PARSER) $(DEBUG) $(HTTP_SERVER) $(REDIS_DEMO)

$(PARSER): parser.cc
	$(CXX) $(CXXFLAGS) $(CPPJIEBA_CFLAGS) -o $@ $^ -lboost_filesystem

$(DEBUG): server.cc $(REDIS_SOURCES)
	$(CXX) $(CXXFLAGS) $(CPPJIEBA_CFLAGS) $(JSONCPP_CFLAGS) $(REDIS_FLAGS) -o $@ $^ -pthread $(JSONCPP_LIBS) $(REDIS_LIBS)

$(HTTP_SERVER): http_server.cc $(REDIS_SOURCES)
	$(CXX) $(CXXFLAGS) $(CPPJIEBA_CFLAGS) $(JSONCPP_CFLAGS) $(REDIS_FLAGS) -o $@ $^ -pthread -lboost_system $(JSONCPP_LIBS) $(REDIS_LIBS)

$(REDIS_DEMO): test/redis_cache_demo.cc $(REDIS_SOURCES)
	$(CXX) $(CXXFLAGS) $(REDIS_FLAGS) -o $@ $^ -pthread $(REDIS_LIBS)

clean:
	rm -f parser debug http_server redis_cache_demo
