#ifndef SOCKET_HPP
#define SOCKET_HPP

#include "utils.hpp"
#include <arpa/inet.h>
#include <math.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

class Socket {
  private:
    struct sockaddr_in target_address;
    int target_socket_fd;

  public:
    Socket();
    Socket(int target_fd);
    Socket(std::string name, uint16_t port);
    ~Socket();
    bool set_target(std::string ip, uint16_t port);
    bool listening(int max_connections);
    Socket *accept_connection();
    bool connect_to_target();
    bool send_message_from(std::string buffer);
    int receive_message_on(std::string &buffer);
};

#endif