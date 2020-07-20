#include "socket.hpp"
#include <cstdlib>
#include <iostream>

// Default constructor. Not used here.
Socket::Socket() {}

/*
 *   Initializes a socket object with a previous allocated socket.
 *
 *   Parameters:
 *      target_fd (int): the previous allocated socket file descriptor
 */
Socket::Socket(int fd, struct sockaddr_in address) {
    this->my_fd = fd;
    this->address = address;
    int optval = 1;
    setsockopt(this->my_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

int Socket::get_my_fd() const { return this->my_fd; }

/*
 *   Sets the ip and the port of the target socket.
 *
 *   Parameters:
 *       ip (string): the ip to be setted; if "any" set it to INADDR_ANY; if "localhost" set it to 127.0.0.1.
 *       port (uint16_t): the port to be setted.
 *   Returns:
 *       bool: whether or not the operation was successful.
 */
bool Socket::set_address(std::string ip, uint16_t port) {
    this->address.sin_port = htons(port);

    if (ip == "any")
        this->address.sin_addr.s_addr = INADDR_ANY;
    else {
        if (ip == "localhost")
            ip = "127.0.0.1";
        int status = inet_aton(ip.c_str(), &(this->address.sin_addr));
        check_error(status, 0, "Conversion of IP failed");
        return status == 0 ? false : true;
    }
    return true;
}

/*
 *   Initializes a socket object with an specific address (IP + port) as it's target.
 *   Please refer to "set_target" documentation for more information on format
 *   and treatment of the parameters.
 */
Socket::Socket(std::string ip, uint16_t port) {
    this->my_fd = socket(AF_INET, SOCK_STREAM, 0); // Using TCP Protocol
    this->address.sin_family = AF_INET;
    if (!this->set_address(ip, port)) {
        throw("Could not create socket (invalid IP).");
    }
    int optval = 1;
    setsockopt(this->my_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

/*
 *   Makes the socket enters into a listening state, with 'max_connections' connections at the same time.
 *
 *   Parameters:
 *       max_connections (int): the maximum number of connections that the socket can hold at any given time.
 *   Returns:
 *       bool: whether or not the operation was successful.
 */
bool Socket::listening(int max_connections) {
    int status_bind = bind(this->my_fd, (struct sockaddr *)&(this->address), sizeof(this->address));
    check_error(status_bind, -1, "Binding failed");
    int status_listen = listen(this->my_fd, max_connections);
    check_error(status_listen, -1, "Listening failed");

    if (status_bind == -1 || status_listen == -1)
        return false;
    return true;
}

/*
 *   Waits until a new connection is tried by a client application.
 *   Then, allocates a new socket to conversate with that client.
 *   Note that the new socket may be in another port
 *   (i.e. different from where the listening socket was estabilished on).
 *
 *   Returns:
 *       Socket: the target socket, now connected.
 */
Socket *Socket::accept_connection() {
    struct sockaddr_in peer_addr;
    socklen_t addr_size = sizeof(peer_addr);

    int new_socket_fd = accept(this->my_fd, (sockaddr *)&peer_addr, &addr_size);
    Socket *s = new Socket(new_socket_fd, peer_addr);
    return s;
}

// Get back the IP of this socket's address
std::string Socket::get_IP_address() {
    struct in_addr net_addr = this->address.sin_addr;
    char ipv4[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &net_addr, ipv4, sizeof(ipv4));
    return std::string(ipv4);
}

/*
 *   Try a connection with the target socket.
 *
 *   Returns:
 *       bool: whether or not the operation was successful.
 */
bool Socket::connect_to_address() {
    int status = connect(this->my_fd, (struct sockaddr *)&(this->address), sizeof(this->address));
    check_error(status, -1, "Connection failed");
    return status == -1 ? false : true;
}

/*
 *   Try to send a message through the socket.
 *
 *   Parameters:
 *      buffer (string): The text to be sent.
 *   Returns:
 *      bool: whether or not the operation was successful.
 */
bool Socket::send_message(std::string buffer) {
    const char *msg = buffer.c_str();

    int status = send(this->my_fd, msg, strlen(msg) + 1, 0);
    check_error(status, -1, "Message delivery failed");
    if (status == -1)
        return false; // Error
    return true;
}

/*
 *   Receives a message from the target socket.
 *
 *   Parameters:
 *      buffer (string): the message will be saved here.
 *   Returns:
 *      int: the number of bytes read. Returns -1 in case of an error.
 */
int Socket::receive_message(std::string &buffer) {
    const char *c_msg = new char[MSG_SIZE + NICK_SIZE + 1]; // +1 because of \0

    int status = recv(this->my_fd, (void *)c_msg, (MSG_SIZE + 1) * sizeof(char), 0);
    check_error(status, -1, "Failed to receive message");

    if (status != -1) {
        if ((int)buffer.size() != status)
            buffer.resize(status);

        for (int i = 0; i < status; i++) {
            buffer[i] = c_msg[i];
            if (buffer[i] == '\0') // Avoid unnecessary iterations
                break;
        }
    }

    delete[] c_msg;
    return status;
}

// Destructor method
Socket::~Socket() { close(this->my_fd); }