#include <arpa/inet.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER_PORT 9000
using namespace std;

int main() {

    int listener_socket;
    int client_socket;
    char buffer[4096]; // PARALELIZAR O BUFFER DEPOIS
    struct sockaddr_in server_address;

    listener_socket = socket(AF_INET, SOCK_STREAM, 0);

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    bind(listener_socket, (struct sockaddr *)&server_address,
         sizeof(server_address));

    listen(listener_socket, 5);

    cout << "Pai tá online\n\n";

    while (1) {
        client_socket = accept(listener_socket, NULL, NULL);
        int msg_len = recv(client_socket, buffer, sizeof(buffer), 0);
        send(client_socket, buffer, msg_len, 0); // echo
        puts(buffer);
        fflush(stdout);
    }

    close(listener_socket);

    return 0;
}