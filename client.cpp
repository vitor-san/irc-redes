#include "socket.hpp"
#include "utils.hpp"
#include <csignal>
#include <iostream>
#include <mutex>
#include <netdb.h>
#include <thread>

using namespace std;

bool running = true;

void handleCtrlC(int signum) { cout << "\nPlease, use the /quit command.\n"; }

void send_message(Socket *s) {
    string buffer, cmd;
    regex r(RGX_CMD); // RGX_CMD defined in "utils.hpp"
    smatch m;
    vector<string> chunks;

    while (running) {
        // Get the message from the client and store it in the buffer
        getline(cin, buffer);
        // Parses the message, searching for commands
        regex_search(buffer, m, r);
        cmd = m[1].str(); // Gets first command found, if any
        // If any command where found (following RGX_CMD rules), then execute it
        if (cmd != "") {
            if (cmd == "quit") {
                running = false;
                exit(EXIT_SUCCESS);
            }
        } else {
            // Regular message
            chunks = break_msg(buffer);
            for (int i = 0; i < (int)chunks.size(); i++) {
                // Send the message to the server
                cout << chunks[i] << "\n";
                s->send_message_from(chunks[i]);
            }
        }
    }
}

void receive_message(Socket *s) {
    string buffer;
    // while an error or quit doesn't occur
    while (running && (s->receive_message_on(buffer) != -1)) {
        cout << "Server:" << buffer << endl;
    }
}

int main(int argc, const char **argv) {
    uint16_t server_port;
    string server_ip, in_buff, out_buff;

    // Check if args are valid
    if (argc < 3) {
        fprintf(stderr, "Not enough arguments.\nUsage format: %s {host-ip} {port}\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Register signal SIGINT and signal handler
    signal(SIGINT, handleCtrlC);

    // Convert the port to an integer
    server_port = atoi(argv[2]);
    // Store server's IP address
    server_ip = string(argv[1]);

    // Create a socket
    Socket my_socket(server_ip, server_port);

    // Attempts to connect to the target (a server)
    my_socket.connect_to_target();

    // Create two threads, one for receiving and one for sending messages
    thread send_t(send_message, &my_socket);
    thread receive_t(receive_message, &my_socket);

    send_t.join();
    receive_t.join();

    return 0;
}