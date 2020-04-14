#include <arpa/inet.h>
#include <iostream>
#include <netdb.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

/*
 *   Check for errors. If any, print them to stderr and exit the program.
 *   Parameters:
 *       status (int): status to be checked (only -1 represents an error).
 *       msg (const char array): message to be printed to stderr if an error is
 *   encountered.
 */
void check_error(int status, const char *msg) {
    if (status == -1) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, const char **argv) {

    int my_socket, server_port;
    char buffer_in[4096];
    char buffer_out[4096];
    struct sockaddr_in server_address;
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

    // Create a socket
    my_socket = socket(AF_INET, SOCK_STREAM, 0);
    check_error(my_socket, "Socket creation failed");

    // Specify server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Gets the message from the client
    cout << "Enter in some data: ";
    cin >> buffer_out;

    int status = connect(my_socket, (struct sockaddr *)&server_address,
                         sizeof(server_address));
    check_error(status, "Connection failed");

    // Sends the message to the server
    send(my_socket, &buffer_out, sizeof(buffer_out), 0);

    // Gets response from server
    recv(my_socket, &buffer_in, sizeof(buffer_in), 0);

    cout << "Response from server: " << buffer_in << endl;

    close(my_socket);

    return 0;
}