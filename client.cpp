#include "defs.h"
#include <arpa/inet.h>
#include <iostream>
#include <netdb.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

int main(int argc, const char **argv) {

    int my_socket, server_port;
    struct sockaddr_in server_address;
    buffer buff_in, buff_out;
    string message = "";
    // struct hostent *hosts;

    // Check if input is valid
    if (argc < 3) {
        fprintf(stderr,
                "Not enough arguments. Usage format: %s {hostname} {port}\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }
    // Converts the port to an integer
    server_port = atoi(argv[2]);
    // Stores server name to connect to
    // hosts =

    // Creates the input and output buffers
    buff_in = new_buff();
    buff_out = new_buff();

    // Create a socket
    my_socket = socket(AF_INET, SOCK_STREAM, 0);
    check_error(my_socket, "Socket creation failed");

    // Specify server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Gets the message from the client
    cout << "Enter a message: ";
    getline(cin, message);
    put_buff(buff_out, message);

    int status = connect(my_socket, (struct sockaddr *)&server_address,
                         sizeof(server_address));
    check_error(status, "Connection failed");

    // Sends the message to the server
    send(my_socket, buff_out.data(), BUFF_SIZE, 0);

    // Gets response from server
    recv(my_socket, buff_in.data(), BUFF_SIZE, 0);

    cout << "Response from server: " << buff_in.data() << endl;

    close(my_socket);

    return 0;
}