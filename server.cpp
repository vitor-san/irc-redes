#include "socket.hpp"
#include "utils.hpp"
#include <iostream>

using namespace std;

int main(int argc, const char **argv) {
    uint16_t server_port;
    string in_buff, out_buff; // We need to replicate these for each connection in module 2

    // Check if args are valid
    if (argc < 2) {
        fprintf(stderr, "Not enough arguments.\nUsage format: %s {port}\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // Convert the port to an integer
    server_port = atoi(argv[1]);

    // Creates a socket that is only going to listen for incoming connection attempts
    Socket listener("any", server_port);

    // Open server for, at most, 10 connections (not useful for now)
    listener.listening(10);

    // Note that, for now, only one client can conversate with the server at any given time.
    Socket *client;
    while (true) {
        // Wait until a new connection arrives. Then, create a new Socket for conversating with this client
        client = listener.accept_connection();
        // Get all messages sent by the client (remember that longer messages will be split in smaller chunks)
        while (client->receive_message_on(in_buff)) {
            // Tries to echo the message chunk to the client
            bool success = client->send_message_from(in_buff);
            if (!success) {    // The server wasn't able to send the message
                delete client; // Close this connection
                continue;
            }
            cout << in_buff << "\n\n"; // Let's see what is being sent
        }
        delete client; // Client quited
    }

    return 0;
}