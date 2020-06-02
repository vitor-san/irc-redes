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
#define PORT 9001
#define PASSWORD "kalinka@SSC0142"

#define nick(tup) get<0>(tup)
#define alive(tup) get<1>(tup)
#define allowed(tup) get<2>(tup)

using hash_value = tuple<string, bool, bool>; // nickname, alive and allowed
using client_hash = unordered_map<Socket *, hash_value>;

struct msg_info {
    // channel
    string content;
};

// class Channel {

// };

class Server {
  private:
    // Atributes
    Socket listener;
    client_hash client_lookup; // Map a client to their information
    queue<msg_info> msg_queue; // Queue of messages to send in broadcast
    mutex mtx;                 // Control of critical regions. Resembles a semaphore.

    // Methods
    void set_nickname(Socket *client, string &new_nick);
    void set_alive(hash_value &cli, bool is_alive);
    void check_password(Socket *client);
    string prepare_msg(string &chunk, Socket *client);
    bool send_chunk(string &chunk, Socket *client);
    void send_to_queue(msg_info &pack);

  public:
    // Methods
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

/* ---------------------------- PRIVATE METHODS ----------------------------- */

// Changes the nickname from a client
void Server::set_nickname(Socket *client, string &new_nick) {

    msg_info msg_pack;
    bool in_use = false;
    hash_value &myself = this->client_lookup[client];
    string size_error_msg("You need to provide a nickname with at least 5 and at most 50 characters.");
    string in_use_error_msg("Nickname already in use.");

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
        this->send_to_queue(msg_pack);
        nick(myself) = new_nick;
    }
}

void Server::set_alive(hash_value &cli, bool is_alive) {
    // Prevent conflicts
    this->mtx.lock();
    alive(cli) = is_alive;
    this->mtx.unlock();
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

/*
    Just adds the nickname of the client to the message.
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

/*
    Tries to send the message chunk to the client.
    Returns false in case of error
*/
bool Server::send_chunk(string &chunk, Socket *client) {
    bool success = false;
    int status;
    string buffer, cmd;
    regex r(RGX_CMD); // RGX_CMD defined in "utils.hpp"
    smatch m;

    for (int t = 0; t < MAX_RET; t++) {
        success = client->send_message_from(chunk);
        if (success) {
            status = client->receive_message_on(buffer);
            if (status > 0) {
                // Parses the message, searching for commands
                regex_search(buffer, m, r);
                // Gets first command found, if any
                cmd = m[1].str();
                if (cmd == "ack") {
                    return true;
                    cout << "CLIENTE RECEBEU" << endl;
                }
            }
        }
    }
    return false;
}

void Server::send_to_queue(msg_info &pack) {
    // Prevent conflicts
    this->mtx.lock();
    this->msg_queue.push(pack);
    this->mtx.unlock();
}

/* ---------------------------- PUBLIC METHODS ------------------------------ */

// Creates a socket that is only going to listen for incoming connection attempts
Server::Server(int port) : listener("any", port) {
    bool success = false;
    // Open server for, at most, MAX_CONN connections
    success = this->listener.listening(MAX_CONN);
    if (!success) {
        throw("Port already in use.");
    }
}

// Method to handle messages received from a specific client (socket)
void Server::receive(Socket *client) {

    string buffer, cmd, new_nick;
    string pong_msg("pong");
    string quit_msg("You have quited successfully!");
    regex r(RGX_CMD); // RGX_CMD defined in "utils.hpp"
    smatch m;
    int status;
    hash_value &myself = this->client_lookup[client];
    msg_info msg_pack;

    // Check if this client knows the password
    // this->check_password(client);
    allowed(myself) = true;

    client->send_message_from(string("\nWelcome to our server!\n\n"));
    msg_pack.content = "A new user has entered the chat!";
    this->send_to_queue(msg_pack);

    while (alive(myself)) {
        // Receive next message
        status = client->receive_message_on(buffer);
        if (status <= 0) {
            // An error ocurred
            break;
        }
        // Parses the message, searching for commands
        regex_search(buffer, m, r);
        cmd = m[1].str(); // Gets first command found, if any
        // If any command where found (following RGX_CMD rules), then execute it
        if (cmd != "") {
            if (cmd == "quit") {
                set_alive(myself, false);
                this->send_chunk(quit_msg, client);
                // Log
                cout << "Client " << nick(myself) << " quited" << endl;
            } else if (cmd == "ping") {
                // Send "pong" to the client
                set_alive(myself, this->send_chunk(pong_msg, client));
                // Log
                if (alive(myself)) {
                    cout << "Pong sent to client " << nick(myself) << endl;
                }
            } else if (cmd == "nickname") {
                // Get the nickname from the message
                new_nick = m[2].str();
                this->set_nickname(client, new_nick);
            }
        } else {
            // The server just got a regular message
            cout << buffer << endl;
            msg_pack.content = this->prepare_msg(buffer, client);
            this->send_to_queue(msg_pack);
            // Erase buffer to avoid headaches
            buffer.clear();
        }
    }

    cout << "Connection closed with client " << nick(myself) << endl;
    msg_pack.content = "User " + nick(myself) + " has disconnected from the server...";
    this->send_to_queue(msg_pack);
    this->client_lookup.erase(client);
}

// Method for accepting new clients (socket connections)
void Server::accept() {
    Socket *client;
    string nickname = "unknown";
    thread new_thread;

    cout << "Now accepting new connections...\n";

    while (true) {
        // Wait until a new connection arrives. Then, create a new Socket for conversating with this client
        client = this->listener.accept_connection();
        // Open a thread to handle messages sent by this client
        new_thread = this->receive_thread(client);
        new_thread.detach();
        // Register they and two control booleans in the hash table
        this->client_lookup.insert(make_pair(client, make_tuple(nickname, true, false)));
        cout << "Inserted client " << nickname << "\n";
    }
}

// Method for broadcasting messages from the queue
void Server::broadcast() {
    msg_info next_msg_pack;
    bool success = false;

    cout << "Now broadcasting messages...\n";

    while (true) {
        // Prevent conflicts
        this->mtx.lock();

        // Only run when queue has something on it
        if (!this->msg_queue.empty()) {
            // Get next message
            next_msg_pack = this->msg_queue.front();
            this->msg_queue.pop();

            // Only send it if it's content is not empty
            if ((int)next_msg_pack.content.size() > 0) {
                // Send it to everyone...
                for (auto it = this->client_lookup.begin(); it != client_lookup.end(); it++) {
                    hash_value &client = this->client_lookup[it->first];

                    // If they are allowed to.
                    if (allowed(client)) {
                        success = this->send_chunk(next_msg_pack.content, it->first);
                        // If we could not send the message to the client, it has quitted from the server
                        if (!success) {
                            alive(client) = false;
                        }
                    }
                }
            }
        }

        this->mtx.unlock();
    }
}

/* ---------------------------- DRIVER FUNCTION ----------------------------- */

int main() {
    Server IRC(PORT);

    thread accept_t = IRC.accept_thread();
    thread broadcast_t = IRC.broadcast_thread();

    accept_t.join();
    broadcast_t.join();

    return 0;
}