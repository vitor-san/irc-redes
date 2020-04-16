#include "socket.hpp"
#include <iostream>

Socket::Socket() {
    this->target_socket_fd = socket(AF_INET, SOCK_STREAM, 6); // Using TCP Protocol
    this->target_address.sin_family = 0;
    this->target_address.sin_port = 0;
    this->target_address.sin_addr.s_addr = 0;
}

Socket::Socket(int target_fd) {
    this->target_socket_fd = target_fd;
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
    this->target_socket_fd = socket(AF_INET, SOCK_STREAM, 6); // Using TCP Protocol
    this->target_address.sin_family = AF_INET;
    if (!this->set_target(ip, port)) {
        throw("Could not create socket (invalid IP).");
    }
}

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

Socket *Socket::accept_connection() {
    int new_socket_fd = accept(this->target_socket_fd, NULL, NULL);
    Socket *s = new Socket(new_socket_fd);
    return s;
}

bool Socket::connect_to_target() {
    int status =
        connect(this->target_socket_fd, (struct sockaddr *)&(this->target_address), sizeof(this->target_address));
    check_error(status, -1, "Connection failed");
    return status == -1 ? false : true;
}

bool Socket::send_message_from(std::string buffer) {
    int msg_len, chunk_len, start, attempts, status;

    msg_len = buffer.size();
    if (buffer[msg_len - 1] == '\0')
        msg_len--;
    // std::cout << "Message size: " << msg_len << std::endl;
    int n_chunks = int(ceil((double)msg_len / MSG_SIZE)); // -1 to make '\0' fit in the end
    // std::cout << "Number of chunks: " << n_chunks << std::endl;

    char chunks[n_chunks][MSG_SIZE + 1];

    for (int i = 0; i < n_chunks; i++) {
        start = i * MSG_SIZE;

        if (i + 1 == n_chunks) // Last chunck
            chunk_len = msg_len - start;
        else
            chunk_len = MSG_SIZE;

        buffer.copy(chunks[i], chunk_len, start);
        chunks[i][chunk_len] = '\0';

        for (attempts = 0; attempts < MAX_RET; attempts++) {
            status = send(this->target_socket_fd, chunks[i], strlen(chunks[i]) + 1, 0);
            check_error(status, -1, "Message delivery failed");
            if (status != -1)
                break; // Message sent successfully
        }

        if (attempts == MAX_RET)
            return false; // Couldn't transmit the message properly

        // std::cout << "Number of attemps: " << attempts + 1 << std::endl; // Remove this later
    }

    return true; // Message transmitted properly
}

int Socket::receive_message_on(std::string &buffer) {
    const char *c_msg = new char[MSG_SIZE + 1];

    int status = recv(this->target_socket_fd, (void *)c_msg, (MSG_SIZE + 1) * sizeof(char), 0);
    check_error(status, -1, "Failed to receive message");

    if (status != -1) {
        if (buffer.size() != status)
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

Socket::~Socket() { close(this->target_socket_fd); }