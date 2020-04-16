#include "socket.hpp"
#include "utils.hpp"
#include <iostream>

using namespace std;

int main(int argc, const char **argv) {

    uint16_t server_port;
    string in_buff, out_buff; // Vao precisar ser replicados para cada conexao depois (quando tiver varios clientes)

    // Check if args are valid
    if (argc < 2) {
        fprintf(stderr, "Not enough arguments.\nUsage format: %s {port}\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // Convert the port to an integer
    server_port = atoi(argv[1]);

    Socket listener("any", server_port);

    // Open server for, at most, 10 connections
    listener.listening(10);

    cout << "Pai tá online.\n"; // Remove this later

    Socket *client;
    while (true) {
        client = listener.accept_connection();
        while (client->receive_message_on(in_buff)) {
            bool success = client->send_message_from(in_buff);
            if (!success) { // The server wasn't able to send the message
                delete client;
                continue;
            }

            cout << "\n" << in_buff << "\n";
        }
        delete client;
    }

    return 0;
}