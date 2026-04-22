PARSER=parser
DUG=debug

.PHONY:all
all:$(PARSER) $(DUG)

$(PARSER):parser.cc
	g++ -o $@ $^ -std=c++11 -lboost_filesystem -fsanitize=address -g
$(DUG):server.cc
	g++ -o $@ $^ -std=c++11 -ljsoncpp -fsanitize=address -g

.PHONY:clean
clean:
	rm -rf parser debug