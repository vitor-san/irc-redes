#include <arpa/inet.h>
#include <array>
#include <iostream>
#include <sstream>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#define SERVER_PORT 9000
#define MSG_SIZE 4096
#define buffer array<char, MSG_SIZE>

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

int main() {

    int listener_socket, client_socket, status, bytes_in;
    struct sockaddr_in server_address;
    vector<buffer> buffers;
    // Maps each socket file descriptor to a index in the buffer vector
    unordered_map<int, int> sock_to_buff_idx;
    const string greetings = "You've successfully connected to an IRC server!";

    // Creates the listener socket
    listener_socket = socket(AF_INET, SOCK_STREAM, 0);
    check_error(listener_socket, "Socket creation failed");

    // Specify server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Binds socket to the server address
    status = bind(listener_socket, (struct sockaddr *)&server_address,
                  sizeof(server_address));
    check_error(status, "Binding failed");

    // Listen for incoming connections, accepting no more than 10 connections at
    // the same time
    status = listen(listener_socket, 10);
    check_error(status, "Listening failed");

    cout << "Pai tá online\n"; // Remove later

    // Creating a set of file descriptors to be monitored over changes (no
    // threads allowed here) and zeroing it
    fd_set master;
    FD_ZERO(&master);
    // Adding the first socket that we're interested in interacting with: the
    // listener socket! It's important that this socket is added for our server
    // or else we won't 'hear' incoming connections
    FD_SET(listener_socket, &master);

    bool running = true; // Only changes because of \quit command

    while (running) {
        // Make a copy of the master file descriptor set because the call to
        // select() is DESTRUCTIVE.
        fd_set master_copy = master;
        // Synchronous I/O multiplexing among all the current fd (see who's
        // talking to us)
        int fd_count = select(0, &master_copy, NULL, NULL, NULL);
        // Loop through all the current/potential connections
        for (const auto &pair : sock_to_buff_idx) {
            int socket = pair.first;
            // Check what kind of socket it is
            if (socket == listener_socket) {
                // Accept a new connection
                int new_socket = accept(listener_socket, NULL, NULL);
                // Add the new connection to the list of connected clients
                FD_SET(new_socket, &master);
                // Associate a new buffer to it and map it's index in the vector
                // to the map
                buffer b;
                b.fill('0');
                buffers.push_back(b);
                sock_to_buff_idx[new_socket] = buffers.size() - 1;
                // Send a welcome message to the connected client
                send(new_socket, greetings.c_str(), greetings.size() + 1, 0);
            } else { // Another socket
                int index = sock_to_buff_idx[socket];
                buffers[index].fill('0');
                char *buf = buffers[index].data();
                // Receive a new message
                bytes_in = recv(socket, buf, MSG_SIZE, 0);
                if (bytes_in <= 0) {
                    // Drop the client
                    close(socket);
                    FD_CLR(socket, &master);
                } else {
                    // Check to see if it's a command. \quit kills the server
                    if (buf[0] == '\\') {
                        // Is the command "quit"?
                        string cmd = string(buf, bytes_in);
                        if (cmd == "\\quit") {
                            running = false;
                            break;
                        }
                        // Unknown command
                        continue;
                    }
                    // Send message to all clients (including the author aswell)
                    for (const auto &pair : sock_to_buff_idx) {
                        int sock = pair.first;
                        if (sock != listener_socket) {
                            ostringstream ss;
                            ss << "SOCKET #" << sock << ": " << buf << endl;
                            string out_str = ss.str();
                            send(sock, out_str.c_str(), out_str.size() + 1, 0);
                        }
                    }
                }
            }
        }
    }

    close(listener_socket);

    return 0;
}