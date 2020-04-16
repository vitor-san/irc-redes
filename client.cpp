#include "socket.hpp"
#include "utils.hpp"
#include <iostream>
#include <netdb.h>
#include <stdlib.h>

using namespace std;

int main(int argc, const char **argv) {

    uint16_t server_port;
    string server_ip, buff_in, buff_out;

    // Check if input is valid
    if (argc < 3) {
        fprintf(stderr, "Not enough arguments. Usage format: %s {hostip} {port}\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // Converts the port to an integer
    server_port = atoi(argv[2]);
    // Stores server name to connect to
    server_ip = string(argv[1]);

    // Create a socket
    Socket my_socket(server_ip, server_port);

    // Gets the buff_in from the client
    cout << "Enter a message: ";
    getline(cin, buff_out);

    my_socket.connect_to_server();

    // Sends the buff_in to the server
    my_socket.send_message(buff_out);

    // Gets response from server
    my_socket.receive_message(buff_in);

    cout << "Response from server: " << buff_in << endl;

    return 0;
}