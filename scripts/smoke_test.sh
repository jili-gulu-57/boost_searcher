#!/usr/bin/env bash
set -euo pipefail

base_url="${1:-http://127.0.0.1:8080}"
keyword="redis-smoke-$(date +%s)"

test "$(curl --silent --show-error "${base_url}/health")" = '{"status":"ok"}'
test "$(curl --silent --show-error --output /dev/null --write-out '%{http_code}' "${base_url}/search")" = '400'

first_cache="$(
  curl --silent --show-error --dump-header - --output /dev/null \
    "${base_url}/search?query=${keyword}" |
    awk 'BEGIN { IGNORECASE=1 } /^X-Cache:/ { gsub("\r", "", $2); print $2 }'
)"
second_cache="$(
  curl --silent --show-error --dump-header - --output /dev/null \
    "${base_url}/search?query=${keyword}" |
    awk 'BEGIN { IGNORECASE=1 } /^X-Cache:/ { gsub("\r", "", $2); print $2 }'
)"

echo "first query X-Cache: ${first_cache}"
echo "second query X-Cache: ${second_cache}"

if [[ "${first_cache}" == "MISS" && "${second_cache}" == "HIT" ]]; then
  echo "Redis search-result cache is working."
elif [[ "${first_cache}" == "BYPASS" && "${second_cache}" == "BYPASS" ]]; then
  echo "Redis is unavailable or disabled; local-search fallback is working."
else
  echo "Unexpected cache status sequence." >&2
  exit 1
fi

hot_status="$(curl --silent --show-error --output /tmp/boost_searcher_hot.json --write-out '%{http_code}' "${base_url}/hot_keywords?limit=10")"
if [[ "${hot_status}" == "200" ]]; then
  echo "hot keywords: $(cat /tmp/boost_searcher_hot.json)"
else
  echo "hot-keyword endpoint returned HTTP ${hot_status}, expected when Redis is unavailable."
fi

echo "boost_searcher smoke test passed"
