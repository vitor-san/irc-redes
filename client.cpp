#include "socket.hpp"
#include "utils.hpp"
#include <iostream>
#include <netdb.h>

using namespace std;

int main(int argc, const char **argv) {

    uint16_t server_port;
    string server_ip, in_buff, out_buff;

    // Check if args are valid
    if (argc < 3) {
        fprintf(stderr, "Not enough arguments.\nUsage format: %s {host-ip} {port}\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // Convert the port to an integer
    server_port = atoi(argv[2]);
    // Store server's IP address
    server_ip = string(argv[1]);

    // Create a socket
    Socket my_socket(server_ip, server_port);

    // Get the message from the client and store it in the output buffer
    cout << "Enter a message: ";
    getline(cin, out_buff);

    my_socket.connect_to_target();

    // Send the message to the server
    my_socket.send_message_from(out_buff);

    // Get response from server
    cout << "Response from server...\n\n";
    while (my_socket.receive_message_on(in_buff) > 0) {
        cout << in_buff << "\n\n";
    }

    return 0;
}