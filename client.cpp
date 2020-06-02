#include "socket.hpp"
#include "utils.hpp"
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <netdb.h>
#include <thread>

using namespace std;

bool running = true;

void handleCtrlC(int signum) { cout << "\nPlease, use the /quit command.\n"; }

// Thread to send messages to the server
void send_message(Socket *s) {
    string buffer, cmd;
    string quit_msg("/quit");
    regex r(RGX_CMD); // RGX_CMD defined in "utils.hpp"
    smatch m;
    vector<string> chunks;
    bool success = false;

    while (running) {
        // Get the message from the client and store it in the buffer
        if (getline(cin, buffer)) {
            // Parses the message, searching for commands
            regex_search(buffer, m, r);
            cmd = m[1].str(); // Gets first command found, if any
            // If any command where found (following RGX_CMD rules), then execute it
            if (cmd != "") {
                if (cmd == "quit") {
                    running = false;
                }
                success = s->send_message_from(buffer);
                while (!success) {
                    success = s->send_message_from(buffer);
                }
            } else {
                // Regular message
                chunks = break_msg(buffer);
                for (int i = 0; i < (int)chunks.size(); i++) {
                    // Send the message to the server
                    success = s->send_message_from(chunks[i]);
                    while (!success) {
                        success = s->send_message_from(buffer);
                    }
                }
            }
        } else {
            // Something has ocurred while reading stdin (EOF or another error)
            running = false;
            success = s->send_message_from(quit_msg);
            while (!success) {
                // The server has to receive the /quit message, otherwise de client
                // will not be erased from the client hash table.
                success = s->send_message_from(quit_msg);
            }
        }
    }
}

// Thread for receiving messages from the server
void receive_message(Socket *s) {
    string buffer;
    int bytes_read = 0;

    while (running) {
        bytes_read = s->receive_message_on(buffer);

        if (bytes_read <= 0) {
            throw("Error while reading messages from server.");
            running = false;
        }

        cout << buffer << endl;
    }
}

void list_servers(server_dns &DNS) {
    cout << endl;
    for (auto it = DNS.begin(); it != DNS.end(); it++) {
        cout << it->first << endl;
    }
    cout << endl;
}

/*
 * Function used to populate the server's IP and port.
 * Returns whether the connection was successful.
 */
bool connect_to(server_dns &DNS, string &server_name, string &ip, uint16_t &port) {
    if ((int)server_name.size() < 5 || (int)server_name.size() > 50)
        return false;
    // Check if the server is registered in our DNS
    if (DNS.count(server_name)) {
        server_data value = DNS[server_name];
        ip = value.first;
        port = value.second;
        return true;
    }
    return false;
}

int main(int argc, const char **argv) {
    string server_name, server_ip, cmd;
    uint16_t server_port;
    regex r(RGX_CMD); // RGX_CMD defined in "utils.hpp"
    smatch m;
    char quit = ' ';
    bool is_in_table, connected = false;
    server_dns DNS = get_dns();
    Socket *my_socket;

    system("clear");
    cout << "Welcome to GG Club.\n\n";

    while (running) {
        cout << "Please, connect to one of our servers using the /connect command.\n"
             << "You can run /list for a list of available servers.\n\n";
        getline(cin, cmd);
        // Parses the input, searching for commands
        regex_search(cmd, m, r);
        cmd = m[1].str(); // Gets command found, if any
        if (cmd == "connect") {
            server_name = m[2].str();
            // Get the IP and the port from the DNS table
            is_in_table = connect_to(DNS, server_name, server_ip, server_port);
            if (!is_in_table) {
                cout << "\nCould not find the specified server in our DNS table.\n\n";
                continue;
            }
            cmd.clear();
        } else if (cmd == "list") {
            list_servers(DNS);
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