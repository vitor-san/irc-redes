FLAGS = -std=c++11 -pthread -lpthread -g -O2

all: server client

server: server.cpp socket.cpp utils.cpp
	g++ $(FLAGS) $^ -o $@

client: client.cpp socket.cpp utils.cpp
	g++ $(FLAGS) $^ -o $@
