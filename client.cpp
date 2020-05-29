#include "socket.hpp"
#include "utils.hpp"
#include <csignal>
#include <cstdlib>
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
        }
        // Regular message
        chunks = break_msg(buffer);
        for (int i = 0; i < (int)chunks.size(); i++) {
            // Send the message to the server
            s->send_message_from(chunks[i]);
        }
    }
}

void receive_message(Socket *s) {
    string buffer;
    // while an error or quit doesn't occur
    while (running && (s->receive_message_on(buffer) > 0)) {
        cout << buffer << endl;
    }
}

int main(int argc, const char **argv) {
    uint16_t server_port;
    string server_ip, cmd;
    char quit = ' ';
    regex r(RGX_CMD); // RGX_CMD defined in "utils.hpp"
    smatch m;
    bool connected = false;
    Socket *my_socket;

    system("clear");
    cout << "Welcome to IRC Club.\n\n";

    while (running) {
        cout << "Please, connect to one of our servers using the /connect command.\n"
             << "You can run the command /list for a list of available servers.\n\n";
        cin >> cmd;
        // Parses the input, searching for commands
        regex_search(cmd, m, r);
        cmd = m[1].str(); // Gets command found, if any
        if (cmd == "connect") {
            cout << "\nEnter the IP of the server (with dots):\n";
            cin >> server_ip;
            cout << "Enter the Port of the server:\n";
            cin >> server_port;
        } else if (cmd == "list") {
            cout << "TROLLADO! Esse comando não foi implementado ainda.\n\n";
            continue;
        } else {
            cout << "Please, provide a valid command for starting.\n\n";
            continue;
        }
        // Create a socket
        my_socket = new Socket(server_ip, server_port);
        // Attempts to connect to the target (a server)
        connected = my_socket->connect_to_target();

        if (connected) {
            system("clear");
            // Register signal SIGINT and signal handler
            signal(SIGINT, handleCtrlC);
            // Create two threads, one for receiving and one for sending messages
            thread send_t(send_message, my_socket);
            thread receive_t(receive_message, my_socket);
            // Wait until both threads terminate
            send_t.join();
            receive_t.join();
        } else {
            while (quit != 'y' && quit != 'n') {
                cout << "Do you wanna quit? (y/n)" << endl;
                cin >> quit;
                system("clear");
            }
            if (quit == 'y') {
                delete my_socket;
                exit(EXIT_SUCCESS);
            } else {
                quit = ' ';
            }
        }

        signal(SIGINT, SIG_DFL);
        delete my_socket;
    }

    return 0;
}