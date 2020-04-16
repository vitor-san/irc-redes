#include "socket.hpp"

Socket::Socket() {
    this->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    this->target_address.sin_family = 0;
    this->target_address.sin_port = 0;
    this->target_address.sin_addr.s_addr = 0;
}

Socket::Socket(int socket_fd) {
    this->socket_fd = socket_fd;
    this->target_address.sin_family = 0;
    this->target_address.sin_port = 0;
    this->target_address.sin_addr.s_addr = 0;
}

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

Socket::Socket(std::string ip, uint16_t port) {
    this->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    this->target_address.sin_family = AF_INET;
    if (!this->set_target(ip, port)) {
        throw("Could not create socket (invalid IP).");
    }
}

bool Socket::open_listen(int max_connections) {
    int status_bind = bind(this->socket_fd, (struct sockaddr *)&(this->target_address), sizeof(this->target_address));
    check_error(status_bind, -1, "Binding failed");
    int status_listen = listen(this->socket_fd, max_connections);
    check_error(status_listen, -1, "Listening failed");

    if (status_bind == -1 || status_listen == -1)
        return false;
    return true;
}

Socket *Socket::accept_connection() {
    int client_socket = accept(this->socket_fd, NULL, NULL);
    Socket *s = new Socket(client_socket);
    return s;
}

bool Socket::connect_to_server() {
    int status = connect(this->socket_fd, (struct sockaddr *)&(this->target_address), sizeof(this->target_address));
    check_error(status, -1, "Connection failed");
    return status == -1 ? false : true;
}

int Socket::send_message(std::string message) {
    int status = send(this->socket_fd, message.c_str(), (message.size() + 1) * sizeof(char), 0);
    check_error(status, -1, "Message delivery failed");
    return status;
}

int Socket::receive_message(std::string &message) {
    const char *c_msg = new char[MSG_SIZE];

    int status = recv(this->socket_fd, (void *)c_msg, MSG_SIZE * sizeof(char), 0);
    check_error(status, -1, "Failed to receive message");

    if (status != -1) {
        if (message.size() != MSG_SIZE)
            message.resize(MSG_SIZE);
        for (int i = 0; i < status; i++) {
            message[i] = c_msg[i];
        }
    }

    delete[] c_msg;
    return status;
}

Socket::~Socket() { close(this->socket_fd); }