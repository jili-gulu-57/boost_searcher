cmake_minimum_required(VERSION 3.16)
project(boost_searcher LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CPPJIEBA_ROOT "${CMAKE_SOURCE_DIR}/cppjieba" CACHE PATH "Path to a cppjieba checkout")
if(NOT EXISTS "${CPPJIEBA_ROOT}/include/cppjieba/Jieba.hpp")
  message(FATAL_ERROR "cppjieba is missing. Set CPPJIEBA_ROOT to a cppjieba checkout.")
endif()

find_package(Boost REQUIRED COMPONENTS filesystem system)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)
pkg_check_modules(HIREDIS QUIET hiredis)
find_package(Threads REQUIRED)

add_library(searcher_redis STATIC
  config.cc
  redis_client.cc
  search_cache.cc
)
target_include_directories(searcher_redis PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(searcher_redis PUBLIC Threads::Threads)
if(HIREDIS_FOUND)
  target_compile_definitions(searcher_redis PRIVATE SEARCHER_ENABLE_REDIS)
  target_include_directories(searcher_redis PRIVATE ${HIREDIS_INCLUDE_DIRS})
  target_link_directories(searcher_redis PRIVATE ${HIREDIS_LIBRARY_DIRS})
  target_link_libraries(searcher_redis PUBLIC ${HIREDIS_LIBRARIES})
  message(STATUS "Redis support enabled with hiredis")
else()
  message(WARNING "hiredis not found; Redis support will be disabled and local search remains available")
endif()

add_executable(parser parser.cc)
target_include_directories(parser PRIVATE "${CPPJIEBA_ROOT}/include")
target_link_libraries(parser PRIVATE Boost::filesystem)

add_executable(search_server http_server.cc)
target_include_directories(search_server PRIVATE "${CPPJIEBA_ROOT}/include" ${JSONCPP_INCLUDE_DIRS})
target_link_directories(search_server PRIVATE ${JSONCPP_LIBRARY_DIRS})
target_link_libraries(search_server PRIVATE searcher_redis Boost::system Threads::Threads ${JSONCPP_LIBRARIES})

add_executable(search_cli server.cc)
target_include_directories(search_cli PRIVATE "${CPPJIEBA_ROOT}/include" ${JSONCPP_INCLUDE_DIRS})
target_link_directories(search_cli PRIVATE ${JSONCPP_LIBRARY_DIRS})
target_link_libraries(search_cli PRIVATE searcher_redis Threads::Threads ${JSONCPP_LIBRARIES})

add_executable(redis_cache_demo test/redis_cache_demo.cc)
target_link_libraries(redis_cache_demo PRIVATE searcher_redis)
