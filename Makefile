all: server client

build: main.cpp
	g++ main.cpp -o irc

server: server.cpp defs.cpp
	g++ server.cpp defs.cpp -o server

client: client.cpp defs.cpp
	g++ client.cpp defs.cpp -o client