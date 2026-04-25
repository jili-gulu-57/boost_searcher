PARSER=parser
DUG=debug
HTTP_SERVER=http_server

.PHONY:all
all:$(PARSER) $(DUG) $(HTTP_SERVER)

$(PARSER):parser.cc
	g++ -o $@ $^ -std=c++11 -lboost_filesystem 
$(DUG):server.cc
	g++ -o $@ $^ -std=c++11 -ljsoncpp 
$(HTTP_SERVER):http_server.cc
	g++ -o $@ $^ -std=c++11 -lpthread -ljsoncpp

.PHONY:clean
clean:
	rm -rf parser debug http_server