#include "socket.hpp"
#include "utils.hpp"
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <tuple>
#include <unordered_map>

using namespace std;

#define MAX_CONN 10
#define SERVER_PORT 9001

#define hash_value tuple<string, thread, bool> // nickname, thread and is_alive
#define nick(tup) get<0>(tup)
#define thre(tup) get<1>(tup)
#define alive(tup) get<2>(tup)
#define client_hash unordered_map<Socket *, hash_value>

mutex mtx; // Control of critical regions. Resembles a semaphore.

struct msg_info {
    // channel
    string content;
};

/*
    Just adds the nickname of the client to the message
    Parameters:
        chunk(string): the message
        clients(client_hash*): struct that has the client name
        client(Socket*): The socket of the client
    Returns:
        msg: the string with nickname + message
*/
string prepare_msg(string &chunk, client_hash *clients, Socket *client) {
    string msg = nick((*clients)[client]) + ": " + chunk;
    return msg;
}

/*
    Tries to send the message chunk to the client.
    Returns false in case of error
    WARNING: Only send chunks of, at maximum, 2047 chars.
*/
bool send_chunk(string &chunk, Socket *client) {
    bool success = false;
    for (int t = 0; t < MAX_RET; t++) {
        cout << "Try " << t + 1 << endl;
        success = client->send_message_from(chunk);
        if (success)
            return true;
    }
    return false;
}

void receive_client_thread(Socket *client, client_hash *clients, queue<msg_info> *msg_queue) {
    string buffer, cmd;
    string pongMsg("pong");
    msg_info msg_pack;
    regex r(RGX_CMD); // RGX_CMD defined in "utils.hpp"
    smatch m;
    hash_value &myself = (*clients)[client];
    string client_nick = nick(myself); // Gets client nickname

    while (alive(myself) && client->receive_message_on(buffer) > 0) {
        // Parses the message, searching for commands
        regex_search(buffer, m, r);
        cmd = m[1].str(); // Gets first command found, if any
        // If any command where found (following RGX_CMD rules), then execute it
        if (cmd != "") {
            if (cmd == "quit") {
                mtx.lock();
                alive(myself) = false;
                mtx.unlock();
                cout << "Client" << client_nick << "quited" << endl;
            } else if (cmd == "ping") {
                // Send "pong" to the client
                mtx.lock();
                alive(myself) = send_chunk(pongMsg, client);
                mtx.unlock();
                cout << "Pong sent to client" << client_nick << endl;
            }
        } else {
            // The server just got a regular message
            cout << buffer << endl;
            msg_pack.content = prepare_msg(buffer, clients, client);
            // Prevent conflicts
            mtx.lock();
            msg_queue->push(msg_pack);
            mtx.unlock();
            // Erase buffer to avoid headaches
            buffer.clear();
        }
    }

    cout << "Connection closed with client " << client_nick << endl;
}

void broadcast_thread(client_hash *clients, queue<msg_info> *msg_queue) {
    msg_info next_msg_pack;
    bool success = false;
    hash_value *cli_tuple;

    cout << "Now broadcasting messages...\n";

    while (true) {
        // Prevent conflicts
        mtx.lock();
        // Only run when queue has something on it
        if (!msg_queue->empty()) {
            next_msg_pack = msg_queue->front();
            msg_queue->pop();

            for (auto it = clients->begin(); it != clients->end(); it++) {
                if ((int)next_msg_pack.content.size() > 0) {
                    success = send_chunk(next_msg_pack.content, it->first);
                    if (!success) {
                        cli_tuple = &(*clients)[it->first];
                        mtx.lock();
                        alive(*cli_tuple) = false;
                        mtx.unlock();
                    }
                }
            }
        }
        mtx.unlock();
    }
}

void accept_thread(Socket *listener, client_hash *clients, queue<msg_info> *msg_queue) {
    Socket *client;
    string nickname = "nickname";

    cout << "Now accepting new connections...\n";

    while (true) {
        // Wait until a new connection arrives. Then, create a new Socket for conversating with this client
        client = listener->accept_connection();
        // Open a thread to handle messages sent by this client
        thread receive_t(receive_client_thread, client, clients, msg_queue);
        // Register client, his thread and a control boolean in the hash
        clients->insert(make_pair(client, make_tuple(nickname, move(receive_t), true)));
        cout << "Inserted client " << nickname << "\n";
    }

    for (auto it = clients->begin(); it != clients->end(); it++) {
        // Get tuple from hash table
        hash_value &tup = it->second;
        if (thre(tup).joinable()) {
            thre(tup).join();
        }
    }
}

int main() {
    client_hash client_lookup; // Map a client to a nickname and a thread
    queue<msg_info> msg_queue; // Queue of messages to send in broadcast

    // Creates a socket that is only going to listen for incoming connection attempts
    Socket listener("any", SERVER_PORT);

    // Open server for, at most, MAX_CONN connections
    listener.listening(MAX_CONN);

    // Starting Thread & move the future object in lambda function by reference
    thread accept_t(accept_thread, &listener, &client_lookup, &msg_queue);
    thread broadcast_t(broadcast_thread, &client_lookup, &msg_queue);

    accept_t.join();
    broadcast_t.join();

    return 0;
}