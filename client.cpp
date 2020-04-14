#include <arpa/inet.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER_PORT 80

using namespace std;

int main(int argc, char **argv) {

    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    string message;
    unsigned int server_ip;

    // Gets input from user to determine the server adress
    cout << "IP address of server (without dots): ";
    cin >> server_ip;
    cout << "\n\n";

    // Gets the message from the client
    cin >> message;

    // Create a socket
    int net_socket;
    net_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Specify server address
    struct sockaddr_in server_address = {.sin_family = AF_INET,
                                         .sin_port = htons(SERVER_PORT),
                                         .sin_addr.s_addr = htonl(server_ip)};

    int connect_status = connect(net_socket, (struct sockaddr *)&server_address,
                                 sizeof(server_address));
    // Check for error with connection
    if (connect_status == -1) {
        cerr << "Fudeu\n";
    }

    const char *buff = message.c_str();

    // Send the data from the server
    send(net_socket, &buff, sizeof(buff), 0);

    close(net_socket);
    return 0;
}