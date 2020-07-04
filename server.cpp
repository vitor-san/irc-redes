#include "socket.hpp"
#include "utils.hpp"
#include <chrono>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <vector>

using namespace std;

#define PORT 9001
#define PASSWORD "kalinka@SSC0142"
#define MAX_CONN 10
#define MAX_CHANNEL_LEN 200

#define nick(tup) get<0>(tup)
#define alive(tup) get<1>(tup)
#define allowed(tup) get<2>(tup)
#define muted(tup) get<3>(tup)

using hash_value = tuple<string, bool, bool, bool>; // nickname, alive, allowed and muted
using client_hash = unordered_map<Socket *, hash_value>;

struct msg_info {
    Socket *sender;
    string channel_name; // Only used when there is a Channel notification to be send in broadcast
    string content;
};

struct Channel {
    client_hash members; // Map a client to their information
    Socket *admin;
    // bool isInviteOnly;
};

class Server {
  private:
    // Atributes
    Socket listener;
    unordered_map<Socket *, string> which_channel;
    unordered_map<string, Channel> channels; // Channel name must have a # in the beginning
    queue<msg_info> msg_queue;               // Queue of messages to send in broadcast
    mutex mtx;                               // Control of critical regions. Resembles a semaphore.

    // Methods
    void set_nickname(Socket *client, string &new_nick);
    void set_alive(hash_value &cli, bool is_alive);
    void check_password(Socket *client);
    string prepare_msg(string &chunk, Socket *client);
    bool send_chunk(string &chunk, Socket *client);
    void send_to_queue(msg_info &pack);
    void remove_from_channel(Socket *client);
    bool change_channel(Socket *client, string new_channel);
    void channel_notification(string c_name, string notification);

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

/*
 *  Set the nickname of the client to new_nick, checks if the name is already
 *  in use and if its size is between 5 and 50.
 *
 *  Parameters:
 *      client(Socket*): The socket of the client
 *      new_nick(string): The new nickname of the client
 */
void Server::set_nickname(Socket *client, string &new_nick) {

    msg_info msg_pack;
    bool in_use = false;
    string cur_channel = this->which_channel[client];
    hash_value &myself = this->channels[cur_channel].members[client];
    string size_error_msg("You need to provide a nickname with at least 5 and at most 50 characters.");
    string in_use_error_msg("Nickname already in use.");

    // test if the nick is already in use
    for (auto it = this->channels[cur_channel].members.begin(); it != this->channels[cur_channel].members.end(); it++) {
        hash_value &cli_tuple = this->channels[cur_channel].members[it->first];
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
        msg_pack.sender = client;
        this->send_to_queue(msg_pack);
        nick(myself) = new_nick;
    }
}

/*
 *  Modify the value alive from the client passed.
 *
 *  Parameters:
 *      hash_value& cli: tuple of client to modify his value alive
 *      bool is_alive: new value of alive
 */
void Server::set_alive(hash_value &cli, bool is_alive) {
    // Prevent conflicts
    this->mtx.lock();
    alive(cli) = is_alive;
    this->mtx.unlock();
}

void Server::check_password(Socket *client) {
    string buffer;
    string pass_prompt("Enter the server's password:");

    // Stays in the loop until the correct password is provided
    while (true) {
        this->send_chunk(pass_prompt, client);
        client->receive_message(buffer);
        if (!strcmp(buffer.c_str(), PASSWORD)) {
            return;
        }
    }
}

/*
 *  Just adds the nickname of the client to the message.
 *
 *  Parameters:
 *      chunk(string): the message
 *      client(Socket *): The socket of the client
 *  Returns:
 *      msg: the string with nickname + message
 */

string Server::prepare_msg(string &chunk, Socket *client) {
    string cur_channel = this->which_channel[client];
    string msg = nick(this->channels[cur_channel].members[client]) + ": " + chunk;
    return msg;
}

/*
 *  Tries to send the message chunk to the client.
 *  Returns false in case of error.
 */
bool Server::send_chunk(string &chunk, Socket *client) {
    bool success = false;
    int status;
    string buffer, cmd;
    regex r(RGX_CMD); // RGX_CMD defined in "utils.hpp"
    smatch m;

    for (int t = 0; t < MAX_RET; t++) {
        success = client->send_message(chunk);
        if (success) {
            return true;
        }
    }

    string cur_channel = this->which_channel[client];
    cout << "Failed to deliver message to " << nick(this->channels[cur_channel].members[client])
         << ". Disconnecting it..." << endl;
    return false;
}

/*
 *  Just pushes a message to the message queue
 *  Parameters:
 *      msg_info& pack: the message to be pushed
 */
void Server::send_to_queue(msg_info &pack) {
    // Prevent conflicts
    this->mtx.lock();
    this->msg_queue.push(pack);
    this->mtx.unlock();
}

void Server::remove_from_channel(Socket *client) {

    string my_channel = this->which_channel[client];

    this->channels[my_channel].members.erase(client);

    int members_on_channel = this->channels[my_channel].members.size();
    // If there is nobody on the channel we need to delete it
    if (members_on_channel == 0 && my_channel != "#general") {
        this->channels.erase(my_channel);
    }

    // If client is the admin, choose another member on the channel to be the next admin
    else if (this->channels[my_channel].admin == client) {
        auto next_admin_ptr = this->channels[my_channel].members.begin();
        this->channels[my_channel].admin = next_admin_ptr->first;
        // Let they know that they became the admin
        hash_value &new_admin = this->channels[my_channel].members[next_admin_ptr->first];
        msg_info msg_pack;
        msg_pack.content = nick(new_admin) + " became the new admin!";
        msg_pack.sender = next_admin_ptr->first;
        this->send_to_queue(msg_pack);
    }
}

bool Server::change_channel(Socket *client, string new_channel) {
    // Guard clause
    if (new_channel[0] != '#') {
        // Channel name is not in the correct format
        client->send_message(string("The Channel name should be preceded by a '#'. Ex: /join #test"));
        return false;
    }

    mtx.lock();

    string my_channel = this->which_channel[client];
    hash_value &myself = this->channels[my_channel].members[client];
    muted(myself) = false;

    // Delete the user from the previous channel
    this->remove_from_channel(client);

    // If new channel does not exist
    if (this->channels.find(new_channel) == this->channels.end()) {
        // Server log
        cout << "Didn't find the channel " << new_channel << ", so I'm creating it." << endl;
        // Create it and set the creator as the admin
        Channel c;
        c.admin = client;
        c.members.insert(make_pair(client, myself));
        this->channels[new_channel] = c;
    } else {
        this->channels[new_channel].members.insert(make_pair(client, myself));
    }
    this->which_channel[client] = new_channel;

    mtx.unlock();
    return true;
}

void Server::channel_notification(string c_name, string notification) {
    if ((int)notification.size() == 0)
        return;

    msg_info msg;
    msg.channel_name = c_name;
    msg.sender = nullptr;
    msg.content = notification;
    this->send_to_queue(msg);
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
    // Create channel #general
    Channel gen;
    gen.admin = nullptr;
    this->channels.insert(make_pair("#general", gen));
}

/*
 *  Method to handle messages received from a specific client (socket)
 *
 *  Parameters:
 *      client(Socket*): The socket of the client
 */
void Server::receive(Socket *client) {

    string buffer, cmd, new_nick, new_channel, target_user;
    string pong_msg("pong");
    string quit_msg("You have quited successfully!");
    regex r(RGX_CMD); // RGX_CMD defined in "utils.hpp"
    smatch m;
    msg_info msg_pack;
    int status;

    string my_channel = "#general";
    hash_value &myself = this->channels[my_channel].members[client];

    // Check if this client knows the password
    // this->check_password(client);
    allowed(myself) = true;

    client->send_message(string("Welcome to our server!\n"));

    msg_pack.content = nick(myself) + " has entered the chat!";
    msg_pack.sender = client;
    this->send_to_queue(msg_pack);

    bool life = true;

    while (life) {

        try {
            life = alive(myself);
        } catch (const std::bad_alloc &e) {
            cout << e.what() << endl;
        }

        // Get my current channel
        my_channel = this->which_channel[client];
        hash_value &myself = this->channels[my_channel].members[client];
        // Receive next message
        status = client->receive_message(buffer);
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
            } else if (cmd == "join") {
                // Get the channel name from the message
                new_channel = m[2].str();
                if ((int)new_channel.size() > MAX_CHANNEL_LEN) {
                    client->send_message(string("The name of a Channel can't be greater than " +
                                                to_string(MAX_CHANNEL_LEN) + " characters."));
                } else {
                    if (this->change_channel(client, new_channel)) {
                        // The operation was successful
                        client->send_message(string("You changed from channel " + my_channel + " to " + new_channel));
                        channel_notification(my_channel, string(nick(myself) + " has left the channel."));
                        my_channel = new_channel;
                        hash_value &myself = this->channels[my_channel].members[client];
                        channel_notification(new_channel, string(nick(myself) + " has entered the channel!"));
                    }
                }
            }
            // The client can only do the following commands if they are an admin
            else if (client == this->channels[my_channel].admin) {
                // Get the target to apply the command to
                target_user = m[2].str();
                bool found = false;

                Socket *target;
                // Tries to find the user in the channel
                for (auto &mem_ptr : this->channels[my_channel].members) {
                    if (nick(mem_ptr.second) == target_user) {
                        found = true;
                        target = mem_ptr.first;
                        break;
                    }
                }
                if (!found) {
                    client->send_message(string("The requested user is not on this channel!"));
                } else {
                    hash_value &target_tup = this->channels[my_channel].members[target];
                    if (cmd == "kick") {
                        this->change_channel(target, "#general");
                        target->send_message(string(nick(myself) + " kicked you from the channel."));
                        this->channel_notification(my_channel,
                                                   string(nick(target_tup) + " has been kicked from the channel."));
                    } else if (cmd == "mute") {
                        muted(target_tup) = true;
                        this->channel_notification(my_channel, string(nick(target_tup) + " has been muted."));
                    } else if (cmd == "unmute") {
                        muted(target_tup) = false;
                        this->channel_notification(my_channel, string(nick(target_tup) + " has been unmuted."));
                    } else if (cmd == "whois") {
                        string target_ip = target->get_IP_address();
                        client->send_message(
                            string("User " + nick(target_tup) + " has the IP address " + target_ip + "."));
                    }
                }
            }
        } else {
            // The server just got a regular message
            cout << buffer << endl;
            msg_pack.content = this->prepare_msg(buffer, client);
            msg_pack.sender = client;
            this->send_to_queue(msg_pack);
            // Erase buffer to avoid headaches
            buffer.clear();
        }
    }

    cout << "Connection closed with client " << nick(myself) << endl;

    msg_pack.content = "User " + nick(myself) + " has disconnected from the server...";
    msg_pack.sender = client;
    this->send_to_queue(msg_pack);

    this->channels[my_channel].members.erase(client);
}

/*
    Method for accepting new clients (socket connections)
*/
void Server::accept() {
    Socket *client;
    string nickname = "unknown";
    thread new_thread;

    cout << "Now accepting new connections...\n";

    while (true) {
        // Wait until a new connection arrives. Then, create a new Socket for conversating with this client
        client = this->listener.accept_connection();
        // Register they and two control booleans in the hash table
        // true for alive and false for allowed (client has not given the password)
        this->which_channel[client] = "#general";
        this->channels["#general"].members.insert(make_pair(client, make_tuple(nickname, true, false, false)));
        cout << "Inserted client " << nickname << "\n";
        // Open a thread to handle messages sent by this client
        new_thread = this->receive_thread(client);
        new_thread.detach();
    }
}

/*
    Method for broadcasting messages from the msg_queue
*/
void Server::broadcast() {
    msg_info next_msg_pack;
    bool success = false;
    string c_name; // Channel name

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
                if (next_msg_pack.sender == nullptr) {
                    // This message is to be broadcasted to the channel named in the msg_pack
                    c_name = next_msg_pack.channel_name;
                    // Send it to everyone on that channel...
                    for (auto it = this->channels[c_name].members.begin(); it != this->channels[c_name].members.end();
                         it++) {
                        hash_value &client = this->channels[c_name].members[it->first];
                        // If they are allowed to.
                        if (allowed(client)) {
                            success = this->send_chunk(next_msg_pack.content, it->first);
                            // If we could not send the message to the client, it has quitted from the server
                            if (!success) {
                                try {
                                    alive(client) = false;
                                } catch (const bad_alloc &e) {
                                    cout << "Bad alloc: " << e.what() << endl;
                                }
                            }
                        }
                    }
                } else {
                    // Get channel name
                    string c_name = this->which_channel[next_msg_pack.sender];
                    // Get sender status
                    hash_value &sender = this->channels[c_name].members[next_msg_pack.sender];
                    // If the sender is not muted...
                    if (!muted(sender)) {
                        // Send it to everyone on that channel...
                        for (auto it = this->channels[c_name].members.begin();
                             it != this->channels[c_name].members.end(); it++) {
                            hash_value &client = this->channels[c_name].members[it->first];
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