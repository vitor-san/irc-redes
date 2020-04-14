#include <arpa/inet.h>
#include <errno.h>
#include <iostream>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER_PORT 9000
using namespace std;

int main() {

    int my_socket;
    char buffer[4096];
    struct sockaddr_in server_address;
    struct hostent *hostent_pointer;
    char message[4096];

    // Create a socket
    my_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Specify server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Gets the message from the client
    cout << "Enter in some data: ";
    cin >> message;
    cout << "\n\n\n";

    int status = connect(my_socket, (struct sockaddr *)&server_address,
                         sizeof(server_address));

    // Checks for error with connection
    if (status == -1) {
        cerr << "Cê foi de base ein garotinho.\n";
        return 1;
    }

    // Sends the message to the server
    send(my_socket, &message, sizeof(message), 0);

    // Gets response from server
    recv(my_socket, &buffer, sizeof(buffer), 0);

    cout << "O SERVER MANDOU A SEGUINTE PIKA: " << buffer;

    close(my_socket);

    return 0;
}