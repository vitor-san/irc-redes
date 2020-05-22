#include "socket.hpp"
#include "utils.hpp"
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>

#define MAX_CONN 10
#define SERVER_PORT 9001
#define client_hash unordered_map<Socket *, string>
#define c_socket first
#define c_nick second

using namespace std;

mutex mtx; // Control of critical regions. Resembles a semaphore.

struct msg_info {
    // string nickname_sender;
    string content;
};

// WARNING: Only send chunks of, at maximum, 2047 chars.
bool send_chunk(string &chunk, Socket *client) {
    // Tries to send the message chunk to the client.
    // Returns false in case of error
    return client->send_message_from(chunk);
}

void receive_client_thread(Socket *client, client_hash *clients, queue<msg_info> *msg_queue) {
    string buffer, cmd;
    string pongMsg("pong");
    msg_info msg_pack;
    regex r(RGX_CMD); // RGX_CMD defined in "utils.hpp"
    smatch m;
    bool alive;

    // While an error doesn't occur
    while (client->receive_message_on(buffer) != -1) {
        // Buffer contains a message of, at max, MSG_SIZE chars
        // Parses the message, searching for commands
        regex_search(buffer, m, r);
        cmd = m[1].str(); // Gets first command found, if any
        // If any command where found (following RGX_CMD rules), then execute it
        if (cmd != "") {
            if (cmd == "quit") {
                cout << "Client disconnected" << endl;
                alive = false;
            } else if (cmd == "ping") {
                // Send "pong" to the client
                // FALTA ENVIAR AS 5 VEZES
                alive = send_chunk(pongMsg, client);
                // If there was any error in the message sending, break this connection
            } else if (cmd == "connect") {
                cout << "Connecting to client" << endl;
            }
        } else {
            // The server just got a regular message
            // TODO: Lookup for client nickname and append it to msg_info struct
            msg_pack.content = buffer;
            mtx.lock();
            msg_queue->push(msg_pack);
            mtx.unlock();
        }
        if (!alive) {
            break;
        }
    }
}

void broadcast_thread(client_hash *clients, queue<msg_info> *msg_queue) {
    msg_info next_msg_pack;

    cout << "Now broadcasting messages...\n";

    while (true) {
        mtx.lock(); // Prevent conflicts

        if (!msg_queue->empty()) {
            next_msg_pack = msg_queue->front();
            msg_queue->pop();
        }

        for (auto it = clients->begin(); it != clients->end(); it++) {
            send_chunk(next_msg_pack.content, it->c_socket);
        }

        mtx.unlock();
    }
}

void accept_thread(Socket *listener, client_hash *clients, queue<msg_info> *msg_queue) {
    Socket *client;
    vector<thread> open_threads;

    cout << "Now accepting new connections...\n";

    while (true) {
        // Wait until a new connection arrives. Then, create a new Socket for conversating with this client
        client = listener->accept_connection();
        // --- Handle nicknames here
        // Open a thread to handle messages sent by this client
        thread receive_t(receive_client_thread, client, clients, msg_queue);
        open_threads.push_back(move(receive_t));
    }

    for (thread &t : open_threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

int main() {
    client_hash client_lookup; // Map a client to a nickname
    queue<msg_info> msg_queue; // Queue of messages to send in broadcast

    // Creates a socket that is only going to listen for incoming connection attempts
    Socket listener("any", SERVER_PORT);

    // Open server for, at most, MAX_CONN connections
    listener.listening(MAX_CONN);

    thread accept_t(accept_thread, &listener, &client_lookup, &msg_queue);
    thread broadcast_t(broadcast_thread, &client_lookup, &msg_queue);

    accept_t.join();
    broadcast_t.join();

    return 0;
}