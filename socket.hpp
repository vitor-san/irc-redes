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

/*
 * Generic class for a socket, meant to be used by both server and client applications.
 * Note, however, that some methods are meant to be used only by server applications
 * and others only by client applications. Whenever this differentiation is necessary,
 * a comment succeding the method will be provided in this header file.
 * Use of server meant methods in client applications (and vice-versa) may cause buggy behavior.
 */
class Socket {
  private:
    struct sockaddr_in target_address;
    // struct sockaddr *con_address;
    int my_fd;
    // int peer_fd;
    Socket(); // "Disables" default constructor

  public:
    Socket(int target_fd);
    Socket(std::string name, uint16_t port);
    ~Socket();
    bool set_target(std::string ip, uint16_t port);
    std::string get_client_IP();
    bool listening(int max_connections); // Server-only
    Socket *accept_connection();         // Server-only
    bool connect_to_target();            // Client-only
    bool send_message_from(std::string buffer);
    int receive_message_on(std::string &buffer);
};

#endif