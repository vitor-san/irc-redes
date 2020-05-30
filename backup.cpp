#include "socket.hpp"
#include "utils.hpp"
#include <chrono>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <tuple>
#include <unordered_map>

using namespace std;

#define MAX_CONN 10
#define SERVER_PORT 9001
#define PASSWORD "kalinka@SSC0142"

#define nick(tup) get<0>(tup)
#define alive(tup) get<1>(tup)

using hash_value = tuple<string, bool>; // nickname, thread and is_alive
using client_hash = unordered_map<Socket *, hash_value>;

struct msg_info {
    // channel
    string content;
};

// class Channel {

// };

class Server {
  private:
    Socket listener;
    client_hash client_lookup; // Map a client to a nickname and a thread
    queue<msg_info> msg_queue; // Queue of messages to send in broadcast
    mutex mtx;                 // Control of critical regions. Resembles a semaphore.
    void check_password(Socket *client);
    void change_nickname(string &new_nick, Socket *client);
    string prepare_msg(string &chunk, Socket *client);
    bool send_chunk(string &chunk, Socket *client);
    void send_to_queue(msg_info &pack);

  public:
    Server(int port);
    void broadcast();
    void accept();
    void receive(Socket *client);
    thread broadcast_thread() {
        return thread([=] { broadcast(); });
    }
    thread accept_thread() {
        return thread([=] { accept(); });
    }
    thread receive_thread(Socket *client) {
        return thread([=] { receive(client); });
    }
};

// Creates a socket that is only going to listen for incoming connection attempts
Server::Server(int port) : listener("any", port) {
    // Open server for, at most, MAX_CONN connections
    this->listener.listening(MAX_CONN);
}

/*
    Just adds the nickname of the client to the message
    Parameters:
        chunk(string): the message
        client(Socket*): The socket of the client
    Returns:
        msg: the string with nickname + message
*/
string Server::prepare_msg(string &chunk, Socket *client) {
    string msg = nick(this->client_lookup[client]) + ": " + chunk;
    return msg;
}

void Server::send_to_queue(msg_info &pack) {
    // Prevent conflicts
    this->mtx.lock();
    this->msg_queue.push(pack);
    this->mtx.unlock();
}

/*
    Tries to send the message chunk to the client.
    Returns false in case of error
    WARNING: Only send chunks of, at maximum, MSG_SIZE-1 chars.
*/
bool Server::send_chunk(string &chunk, Socket *client) {
    bool success = false;
    for (int t = 0; t < MAX_RET; t++) {
        success = client->send_message_from(chunk);
        if (success)
            return true;
    }
    return false;
}

void Server::check_password(Socket *client) {
    string buffer;

    // Stays in the loop until the correct password is provided
    while (true) {
        client->send_message_from(string("Enter the server's password:"));
        client->receive_message_on(buffer);
        if (!strcmp(buffer.c_str(), PASSWORD)) {
            return;
        }
    }
}

// Private method to change the nickname from a client
void Server::change_nickname(string &new_nick, Socket *client) {
    msg_info msg_pack;
    string size_error_msg("You need to provide a nickname with at least 5 and at most 50 characters.");
    string in_use_error_msg("Nickname already in use.");
    bool in_use = false;
    hash_value &myself = this->client_lookup[client];

    // test if the nick is already in use
    for (auto it = this->client_lookup.begin(); it != this->client_lookup.end(); it++) {
        hash_value &cli_tuple = this->client_lookup[it->first];
        if (nick(cli_tuple) == new_nick) {
            in_use = true;
            break;
        }
    }

    if (new_nick.size() < 5 || new_nick.size() > 50) {
        // Sends just to this client
        this->send_chunk(size_error_msg, client);
    } else if (in_use) {
        this->send_chunk(in_use_error_msg, client);
    } else {
        // Notice to everyone the change
        msg_pack.content = "User " + nick(myself) + " changed his nickname to " + new_nick + ".";
        // Prevent conflicts
        this->mtx.lock();
        this->msg_queue.push(msg_pack);
        nick(myself) = new_nick;
        this->mtx.unlock();
    }
}

// Method to handle messages received from a specific client (socket)
void Server::receive(Socket *client) {
    string buffer, cmd, new_nick;
    string pongMsg("pong");
    msg_info msg_pack;
    regex r(RGX_CMD); // RGX_CMD defined in "utils.hpp"
    smatch m;
    hash_value &myself = this->client_lookup[client];

    // Check if client knows the password
    this->check_password(client);

    // Gives to the client a generic nickname
    nick(myself) = "unknown";
    // Register client, his thread and a control boolean in the hash
    this->client_lookup.insert(make_pair(client, make_tuple(nick(myself), true)));
    cout << "Inserted client " << nick(myself) << "\n";

    client->send_message_from(string("\n\nWelcome to our server!\n\n"));

    msg_pack.content = "A new user has entered the chat!";

    while (alive(myself) && client->receive_message_on(buffer) > 0) {
        // Parses the message, searching for commands
        regex_search(buffer, m, r);
        cmd = m[1].str(); // Gets first command found, if any
        // If any command where found (following RGX_CMD rules), then execute it
        if (cmd != "") {
            if (cmd == "quit") {
                this->mtx.lock();
                alive(myself) = false;
                this->mtx.unlock();
                cout << "Client" << nick(myself) << "quited" << endl;
            } else if (cmd == "ping") {
                // Send "pong" to the client
                this->mtx.lock();
                alive(myself) = this->send_chunk(pongMsg, client);
                this->mtx.unlock();
                cout << "Pong sent to client " << nick(myself) << endl;
            } else if (cmd == "nickname") {
                // Get the nickname from the message
                new_nick = m[2].str();
                this->change_nickname(new_nick, client);
            }
        } else {
            // The server just got a regular message
            cout << buffer << endl;
            msg_pack.content = this->prepare_msg(buffer, client);
            // Prevent conflicts
            this->send_to_queue(msg_pack);
            // Erase buffer to avoid headaches
            buffer.clear();
        }
    }

    cout << "Connection closed with client " << nick(myself) << endl;
    msg_pack.content = "User " + nick(myself) + " has disconnected from the server...";
    this->send_to_queue(msg_pack);
}

// Method for accepting new clients (socket connections)
void Server::accept() {
    Socket *client;
    vector<thread> open_threads;

    cout << "Now accepting new connections...\n";

    while (true) {
        // Wait until a new connection arrives. Then, create a new Socket for conversating with this client
        client = this->listener.accept_connection();
        // Open a thread to handle messages sent by this client
        thread receive_t = this->receive_thread(client);
        open_threads.push_back(move(receive_t));
    }

    for (auto it = open_threads.begin(); it != open_threads.end(); it++) {
        thread &t = *it;
        if (t.joinable()) {
            t.join();
        }
    }
}

// Method for broadcasting messages from the queue
void Server::broadcast() {
    msg_info next_msg_pack;
    bool success = false;
    hash_value *cli_tuple;

    cout << "Now broadcasting messages...\n";

    while (true) {
        // Prevent conflicts
        this->mtx.lock();
        // Only run when queue has something on it
        if (!this->msg_queue.empty()) {
            next_msg_pack = this->msg_queue.front();
            this->msg_queue.pop();

            for (auto it = this->client_lookup.begin(); it != client_lookup.end(); it++) {
                if ((int)next_msg_pack.content.size() > 0) {
                    success = this->send_chunk(next_msg_pack.content, it->first);
                    // if we could not send the message to the client, it is quitted from the server
                    if (!success) {
                        cli_tuple = &(this->client_lookup)[it->first];
                        this->mtx.lock();
                        alive(*cli_tuple) = false;
                        this->mtx.unlock();
                    }
                }
            }
        }
        this->mtx.unlock();
    }
}

int main() {
    Server IRC(SERVER_PORT);

    thread accept_t = IRC.accept_thread();
    thread broadcast_t = IRC.broadcast_thread();

    accept_t.join();
    broadcast_t.join();

    return 0;
}