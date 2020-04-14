#include <arpa/inet.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv) {
    char buff[4096];

    struct sockaddr_in caddr;
    struct sockaddr_in saddr = {.sin_family = AF_INET,
                                .sin_port = htons(80),
                                .sin_addr.s_addr = htonl(INADDR_ANY)};

    int server = socket(AF_INET, SOCK_STREAM, 0);
    int client, x;
    int csize = sizeof(caddr);

    bind(server, (struct sockaddr *)&saddr, sizeof(saddr));
    listen(server, 5);

    while (1) {
        client = accept(server, (struct sockaddr *)&caddr, &csize);
        x = recv(client, buff, sizeof(buff), 0);

        send(client, buff, x, 0);

        puts(buff);
        fflush(stdout);
    }

    return 0;
}