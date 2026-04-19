parser:parser.cc
	g++ -o $@ $^ -std=c++11 -lboost_filesystem

.PHONY:clean
clean:
	rm -rf parser