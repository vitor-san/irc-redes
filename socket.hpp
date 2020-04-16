#ifndef SOCKET_HPP
#define SOCKET_HPP

#include "utils.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

class Socket {
  private:
    struct sockaddr_in target_address;
    int socket_fd;

  public:
    Socket();
    Socket(int socket_fd);
    Socket(std::string name, uint16_t port);
    ~Socket();
    bool set_target(std::string ip, uint16_t port);
    bool open_listen(int max_connections);
    Socket *accept_connection();
    bool connect_to_server();
    int send_message(std::string message);
    int receive_message(std::string &message);
};

#endif