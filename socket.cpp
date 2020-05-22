#include "socket.hpp"

// Default constructor. Not used here.
Socket::Socket() {}

/*
 *   Initializes a socket object with a previous allocated socket.
 *   Parameters:
 *      target_fd (int): the previous allocated socket file descriptor
 */
Socket::Socket(int target_fd) {
    this->target_socket_fd = target_fd;
    this->target_address.sin_family = 0;
    this->target_address.sin_port = 0;
    this->target_address.sin_addr.s_addr = 0;
}

/*
 *   Sets the ip and the port of the target socket.
 *   Parameters:
 *       ip (string): the ip to be setted; if "any" set it to INADDR_ANY; if "localhost" set it to 127.0.0.1.
 *       port (uint16_t): the port to be setted.
 *   Returns:
 *       bool: whether or not the operation was successful.
 */
bool Socket::set_target(std::string ip, uint16_t port) {
    this->target_address.sin_port = htons(port);

    if (ip == "any")
        this->target_address.sin_addr.s_addr = INADDR_ANY;
    else {
        if (ip == "localhost")
            ip = "127.0.0.1";
        int status = inet_aton(ip.c_str(), &(this->target_address.sin_addr));
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
    this->target_socket_fd = socket(AF_INET, SOCK_STREAM, 6); // Using TCP Protocol
    this->target_address.sin_family = AF_INET;
    if (!this->set_target(ip, port)) {
        throw("Could not create socket (invalid IP).");
    }
}

/*
 *   Makes the socket enters into a listening state, with 'max_connections' connections at the same time.
 *   Parameters:
 *       max_connections (int): the maximum number of connections that the socket can hold at any given time.
 *   Returns:
 *       bool: whether or not the operation was successful.
 */
bool Socket::listening(int max_connections) {
    int status_bind =
        bind(this->target_socket_fd, (struct sockaddr *)&(this->target_address), sizeof(this->target_address));
    check_error(status_bind, -1, "Binding failed");
    int status_listen = listen(this->target_socket_fd, max_connections);
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
 *   Returns:
 *       Socket: the target socket, now connected.
 */
Socket *Socket::accept_connection() {
    int new_socket_fd = accept(this->target_socket_fd, NULL, NULL);
    Socket *s = new Socket(new_socket_fd);
    return s;
}

/*
 *   Try a connection with the target socket.
 *   Returns:
 *       bool: whether or not the operation was successful.
 */
bool Socket::connect_to_target() {
    int status =
        connect(this->target_socket_fd, (struct sockaddr *)&(this->target_address), sizeof(this->target_address));
    check_error(status, -1, "Connection failed");
    return status == -1 ? false : true;
}

/*
 *   Try to send a message through the socket. If the message surpass 2048 characters, it is splited into chunks of 2048
 *   characters each (including \0), and send one by one.
 *   Parameters:
 *      buffer (string): The text to be sent.
 *   Returns:
 *      bool: whether or not the operation was successful.
 */
bool Socket::send_message_from(std::string buffer) {
    const char *msg = buffer.c_str();

    int status = send(this->target_socket_fd, msg, strlen(msg) + 1, 0);
    check_error(status, -1, "Message delivery failed");
    if (status == -1)
        return false; // Error
    return true;
}

/*
 *   Receives a message from the target socket.
 *   Parameters:
 *      buffer (string): the message will be saved here.
 *   Returns:
 *      int: the number of bytes read. Returns -1 in case of an error.
 */
int Socket::receive_message_on(std::string &buffer) {
    const char *c_msg = new char[MSG_SIZE + 1];

    int status = recv(this->target_socket_fd, (void *)c_msg, (MSG_SIZE + 1) * sizeof(char), 0);
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
Socket::~Socket() { close(this->target_socket_fd); }