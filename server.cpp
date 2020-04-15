#include "defs.h"
#include <arpa/inet.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

int main() {

    int listener_socket, client_socket, status, bytes_in;
    struct sockaddr_in server_address;
    buffer buff_in, buff_out;
    const string greetings = "You've successfully connected to an IRC server!";

    // Creates the input and output buffers
    buff_in = new_buff();
    buff_out = new_buff();

    // Creates the listener socket
    listener_socket = socket(AF_INET, SOCK_STREAM, 0);
    check_error(listener_socket, "Socket creation failed");

    // Specify server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Binds socket to the server address
    status = bind(listener_socket, (struct sockaddr *)&server_address,
                  sizeof(server_address));
    check_error(status, "Binding failed");

    // Listen for incoming connections, accepting no more than 10 connections at
    // the same time
    status = listen(listener_socket, 10);
    check_error(status, "Listening failed");

    cout << "Pai tá online\n"; // Remove later

    while (true) {
        client_socket = accept(listener_socket, NULL, NULL);
        int msg_len = recv(client_socket, buff_in.data(), BUFF_SIZE, 0);
        send(client_socket, buff_in.data(), msg_len, 0); // echo
        puts(buff_in.data());
        fflush(stdout);
    }

    close(listener_socket);

    return 0;
}