#include <arpa/inet.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

int main() {
    char buff[4096];
    struct sockaddr_in caddr;
    struct sockaddr_in saddr;

    caddr.sin_family = AF_INET;
    caddr.sin_port = htons(80);
    caddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int server = socket(AF_INET, SOCK_STREAM, 0);
    int client, x;
    socklen_t csize = sizeof(caddr);

    bind(server, (struct sockaddr *)&saddr, sizeof(saddr));
    listen(server, 5);

    cout << "Pai tá online\n\n";

    while (1) {
        client = accept(server, (struct sockaddr *)&caddr, &csize);
        x = recv(client, buff, sizeof(buff), 0);

        send(client, buff, x, 0);

        puts(buff);
        fflush(stdout);
    }

    close(server);

    return 0;
}