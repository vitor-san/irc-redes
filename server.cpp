#include "socket.hpp"
#include "utils.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <map>
#include <mutex>
#include <queue>
#include <set>
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
    bool isInviteOnly = false;
    set<string> invited_users; // Set of nicknames
};

class Server {
  private:
    // Atributes
    Socket listener;
    map<string, Socket *> users;
    unordered_map<Socket *, string> which_channel;
    unordered_map<string, Channel> channels; // Channel name must have a # in the beginning
    queue<msg_info> msg_queue;               // Queue of messages to send in broadcast
    mutex mtx;                               // Control of critical regions. Resembles a semaphore.

    // Methods
    bool set_nickname(Socket *client, hash_value &client_tup, string &new_nick);
    void set_alive(hash_value &cli, bool is_alive);
    void assert_password(Socket *client);
    string assert_nickname(Socket *client);
    bool is_valid_nickname(string &nick, Socket *client);
    string prepare_msg(string &chunk, Socket *client);
    bool send_chunk(string chunk, Socket *client);
    void send_to_queue(msg_info pack);
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

bool Server::is_valid_nickname(string &nick, Socket *client) {
    // Size error
    if (nick.size() < NICK_MIN || nick.size() > NICK_MAX) {
        // Sends just to this client
        this->send_chunk("You need to provide a nickname with at least 5 and at most 50 characters.", client);
        return false;
    }
    // In use error
    if (this->users.find(nick) != this->users.end()) {
        // Sends just to this client
        this->send_chunk("Nickname already in use.", client);
        return false;
    }
    return true;
}

/*
 *  Set the nickname of the client to new_nick, checks if the name is already
 *  in use and if its size is between 5 and 50.
 *
 *  Parameters:
 *      client(Socket*): The socket of the client
 *      client_tup(hash_value): Tuple of the client
 *      new_nick(string): The new nickname of the client
 */
bool Server::set_nickname(Socket *client, hash_value &client_tup, string &new_nick) {
    // Guard clause
    if (!is_valid_nickname(new_nick, client))
        return false;

    // Notice to everyone the change
    string my_channel = this->which_channel[client];
    this->channel_notification(my_channel, "User " + nick(client_tup) + " changed his nickname to " + new_nick + ".");

    // Change the nick
    this->users.erase(nick(client_tup));
    this->users.insert(make_pair(new_nick, client));
    this->channels[my_channel].invited_users.erase(nick(client_tup));
    this->channels[my_channel].invited_users.insert(new_nick);
    nick(client_tup) = new_nick;

    return true;
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

void Server::assert_password(Socket *client) {
    string buffer;
    // Stays in the loop until the correct password is provided
    while (true) {
        this->send_chunk("Enter the server's password:", client);
        client->receive_message(buffer);
        if (!strcmp(buffer.c_str(), PASSWORD)) {
            return;
        }
    }
}

string Server::assert_nickname(Socket *client) {
    string buffer, cmd, nick;
    regex r(RGX_CMD); // RGX_CMD defined in "utils.hpp"
    smatch m;
    bool valid = false;

    // Stays in the loop until the a valid nickname is provided
    while (!valid) {
        client->receive_message(buffer);
        // Parses the message, searching for commands
        regex_search(buffer, m, r);
        // Gets first command found, if any
        cmd = m[1].str();
        if (cmd != "nickname") {
            this->send_chunk("Please provide your nickname only after a /nickname command.", client);
            continue;
        }
        nick = m[2].str();
        valid = this->is_valid_nickname(nick, client);
    }
    // Register nickname
    this->users.insert(make_pair(nick, client));
    // Return the nickname
    return nick;
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
bool Server::send_chunk(string chunk, Socket *client) {
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
void Server::send_to_queue(msg_info pack) {
    // Prevent conflicts
    this->mtx.lock();
    this->msg_queue.push(pack);
    this->mtx.unlock();
}

void Server::remove_from_channel(Socket *client) {

    string my_channel = this->which_channel[client];
    string my_nick = nick(this->channels[my_channel].members[client]);

    // Remove they from the channel
    if (this->channels[my_channel].isInviteOnly) {
        this->channels[my_channel].invited_users.erase(my_nick);
    }
    this->channels[my_channel].members.erase(client);

    int members_on_channel = this->channels[my_channel].members.size();

    // If there is nobody on the channel we need to delete it
    if (members_on_channel == 0 && my_channel != "#general") {
        this->channels.erase(my_channel);
    }

    // If client is the admin, Set the next admin
    else if (this->channels[my_channel].admin == client) {
        Socket *new_adm = this->channels[my_channel].members.begin()->first;
        this->channels[my_channel].admin = new_adm;
        this->send_chunk("You are now the new admin of the channel!", new_adm);
    }
}

bool Server::change_channel(Socket *client, string new_channel) {
    // Guard clause
    if (new_channel[0] != '#') {
        // Channel name is not in the correct format
        this->send_chunk("The Channel name should be preceded by a '#'. Ex: /join #test", client);
        return false;
    }

    mtx.lock();

    string my_channel = this->which_channel[client];
    hash_value myself = this->channels[my_channel].members[client];
    muted(myself) = false;

    // Check if the channel is invite-only and if the user is invited
    if (this->channels.find(new_channel) != this->channels.end()) { // If the channel already exists
        if (this->channels[new_channel].isInviteOnly) {
            if (this->channels[new_channel].invited_users.find(nick(myself)) ==
                this->channels[new_channel].invited_users.end()) {
                // User was not invited to this channel
                this->send_chunk("You were not invited to " + new_channel, client);
                mtx.unlock();
                return false;
            }
        }
    }

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

    string buffer, cmd, new_nick, new_channel, target_user, mode_args;
    regex r(RGX_CMD); // RGX_CMD defined in "utils.hpp"
    smatch m;
    msg_info msg_pack;
    int status;

    // Assert that this client's nickname is valid
    string my_nick = this->assert_nickname(client);
    // Assert that this client knows the password
    // this->assert_password(client);

    // Welcome they in the general channel
    string my_channel = "#general";

    // Register they and three control booleans in the hash table:
    // - true for alive, true for allowed and false for muted.
    this->channels[my_channel].members.insert(make_pair(client, make_tuple(my_nick, true, true, false)));
    this->which_channel[client] = my_channel;

    hash_value *myself = &(this->channels[my_channel].members[client]);

    // Log
    cout << "Inserted client " << my_nick << "\n";

    this->send_chunk("Welcome to our server!\n", client);

    msg_pack.content = my_nick + " has entered the chat!";
    msg_pack.sender = client;
    this->send_to_queue(msg_pack);

    // Loop until client dies
    while (alive(*myself)) {
        // Receive next message
        status = client->receive_message(buffer);
        if (status <= 0) {
            // An error ocurred
            break;
        }
        // Parses the message, searching for commands
        regex_search(buffer, m, r);
        // Gets first command found, if any
        cmd = m[1].str();
        // If any command where found (following RGX_CMD rules), then execute it
        if (cmd != "") {
            if (cmd == "quit") {
                this->remove_from_channel(client);
                // Erase their nickname from the server
                this->users.erase(my_nick);
                set_alive(*myself, false);
                cout << "Client " << my_nick << " quited" << endl; // Log
            } else if (cmd == "ping") {
                // Send "pong" to the client
                set_alive(*myself, this->send_chunk("pong", client));
                // Log
                if (alive(*myself)) {
                    cout << "Pong sent to client " << my_nick << endl;
                }
            } else if (cmd == "nickname") {
                // Get the nickname from the message
                new_nick = m[2].str();
                this->set_nickname(client, *myself, new_nick);
                my_nick = new_nick;
            } else if (cmd == "join") {
                // Get the channel name from the message
                new_channel = m[2].str();
                int new_c_len = (int)new_channel.size();
                if (new_c_len < 1 || new_c_len > MAX_CHANNEL_LEN) {
                    this->send_chunk("The name of a Channel should have between 1 to " + to_string(MAX_CHANNEL_LEN) +
                                         " characters.",
                                     client);
                }
                // Trying to join the current channel
                else if (!new_channel.compare(my_channel)) {
                    this->send_chunk("You're already on that channel.", client);
                }
                // Succesfully try to join a channel
                else {
                    if (this->change_channel(client, new_channel)) {
                        // The operation was successful
                        this->send_chunk("You changed from channel " + my_channel + " to " + new_channel, client);
                        channel_notification(my_channel, my_nick + " has left the channel.");
                        channel_notification(new_channel, my_nick + " has entered the channel!");
                    }
                }
            }
            // The client can only do the following commands if they are an admin
            else if (client == this->channels[my_channel].admin) {
                if (cmd == "mode") {
                    mode_args = m[2].str();
                    if (mode_args == "+i") {
                        this->channels[my_channel].isInviteOnly = true;
                        this->channel_notification(my_channel, "The channel is now invite-only!");
                    } else if (mode_args == "-i") {
                        this->channels[my_channel].isInviteOnly = false;
                        this->channel_notification(my_channel, "The channel is now public!");
                    } else {
                        this->send_chunk("Usage form: /mode (+|-)i", client);
                        if (mode_args[0] != '+' && mode_args[0] != '-') {
                            this->send_chunk("You can only add (+) or delete (-) a mode from a channel", client);
                        }
                        if (mode_args[1] != 'i') {
                            this->send_chunk("The only mode that you can use in this channel is 'i' (invite only)",
                                             client);
                        }
                    }
                } else {
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
                        if (cmd == "invite") {
                            // Only proceed if the user exists in the server
                            if (this->users.find(target_user) != this->users.end()) {

                                if (this->channels[my_channel].isInviteOnly) {
                                    this->channels[my_channel].invited_users.insert(target_user);
                                }
                                this->send_chunk(my_nick + " invited you to join channel " + my_channel,
                                                 this->users[target_user]);
                                this->send_chunk(target_user + " invited to join this channel!", client);
                            } else {
                                this->send_chunk(target_user + " does not exists...", client);
                            }
                        } else {
                            this->send_chunk("The requested user is not on this channel!", client);
                        }
                    } else {
                        hash_value &target_tup = this->channels[my_channel].members[target];
                        // Check if the target of the command is the user itself
                        if (client == target) {
                            this->send_chunk("You can't target yourself using an admin command", client);
                        } else {
                            if (cmd == "kick") {
                                this->change_channel(target, "#general");
                                this->send_chunk(my_nick + " kicked you from the channel.", target);
                                this->channel_notification(my_channel,
                                                           nick(target_tup) + " has been kicked from the channel.");
                            } else if (cmd == "mute") {
                                muted(target_tup) = true;
                                this->channel_notification(my_channel, nick(target_tup) + " has been muted.");
                            } else if (cmd == "unmute") {
                                muted(target_tup) = false;
                                this->channel_notification(my_channel, nick(target_tup) + " has been unmuted.");
                            } else if (cmd == "whois") {
                                string target_ip = target->get_IP_address();
                                this->send_chunk("User " + nick(target_tup) + " has the IP address " + target_ip + ".",
                                                 client);
                            } else if (cmd == "invite") {
                                this->send_chunk(target_user + " is already in the channel!", client);
                            }
                        }
                    }
                }
            } else if (cmd == "invite") {
                // You can't use invite command if you're not the admin
                this->send_chunk("You can't use invite command if you're not the admin of the channel", client);
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
        // Get my current channel and tuple
        my_channel = this->which_channel[client];
        myself = &(this->channels[my_channel].members[client]);
    }

    cout << "Connection closed with client " << my_nick << endl;
    this->channel_notification(my_channel, "User " + my_nick + " has disconnected from the server...");
}

/*
    Method for accepting new clients (socket connections)
*/
void Server::accept() {
    Socket *client;
    thread new_thread;

    cout << "Now accepting new connections...\n";

    while (true) {
        // Wait until a new connection arrives. Then, create a new Socket for conversating with this client
        client = this->listener.accept_connection();
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
                                alive(client) = false;
                            }
                        }
                    }
                } else {
                    // Get channel name
                    string c_name = this->which_channel[next_msg_pack.sender];
                    // Get sender status
                    hash_value &sender = this->channels[c_name].members[next_msg_pack.sender];
                    // If the sender is the admin of the channel
                    if (next_msg_pack.sender == this->channels[c_name].admin) {
                        // Send it to everyone on that channel...
                        for (auto it = this->channels[c_name].members.begin();
                             it != this->channels[c_name].members.end(); it++) {
                            hash_value &client = this->channels[c_name].members[it->first];
                            // If they are allowed to.
                            if (allowed(client)) {
                                success = this->send_chunk("@" + next_msg_pack.content, it->first);
                                // If we could not send the message to the client, it has quitted from the server
                                if (!success) {
                                    alive(client) = false;
                                }
                            }
                        }
                    }
                    // If the sender is not the admin and is not muted...
                    else if (!muted(sender)) {
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