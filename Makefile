all: server client

server: server.cpp socket.cpp utils.cpp
	g++ server.cpp socket.cpp utils.cpp -o server -std=c++11

client: client.cpp socket.cpp utils.cpp
	g++ client.cpp socket.cpp utils.cpp -o client -std=c++11