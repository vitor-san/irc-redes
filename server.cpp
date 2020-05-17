#include "socket.hpp"
#include "utils.hpp"
#include <iostream>
#include <list>
#include <mutex>
#include <queue>
#include <thread>

#define MAX_CONN 10
using namespace std;

struct msg_info {
    string nickname_sender;
    string content;
};

mutex mtx; // Control of critical regions. Resembles a semaphore.

// TO DO:

// FILA DE MENSAGENS:
//-fila de mensagens a serem enviadas para todos -> msg: identificador de quem mandou, msg em si
//-a thread de cada soquete vai mandar para o client com quem esta conectado todas as mensagens desta fila
//-se uma thread receber do cliente uma mensagem, adiciona a fila de mensagens tal mensagem a ser mandada para todos
//-LOGO, NA INSERCAO NA FILA USAR MUTEX -> REGIAO CRITICA

// THREADS
// 1 thread geral para enviar a fila de mensagens em broadcast
// 1 thread para cada socket para receber mensagens

// WARNING: Only send chunks of, at maximum, 2047 chars.
bool send_chunk(string &chunk, Socket *client) {
    // Tries to send the message chunk to the client.
    // Returns false in case of error
    return client->send_message_from(chunk);
}

// THREAD DE CADA SOQUETE
void receive_from_client(Socket *client) {
    string buffer, cmd;
    string pongMsg("pong");
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
            cout << buffer << endl;
            alive = send_chunk(buffer, client);
            // TODO: send broadcast
        }
        if (!alive) {
            break;
        }
    }
}

// THREAD QUE ENVIA EM BROADCAST
void send_message_broadcast() {}

int main(int argc, const char **argv) {
    uint16_t server_port;
    queue<msg_info> msg_queue; // Queue of messages to send in broadcast
    // vector<Socket> clients;

    // Check if args are valid
    if (argc < 2) {
        fprintf(stderr, "Not enough arguments.\nUsage format: %s {port}\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // Convert the port to an integer
    server_port = atoi(argv[1]);

    // Creates a socket that is only going to listen for incoming connection attempts
    Socket listener("any", server_port);

    // Open server for, at most, MAX_CONN connections
    listener.listening(MAX_CONN);

    Socket *client;
    while (true) {
        // Wait until a new connection arrives. Then, create a new Socket for conversating with this client
        client = listener.accept_connection();
        // clients.push_back(*client);
        // Get all messages sent by the client
        receive_from_client(client);
        // thread receive_t(receive_from_client, client);
        // receive_t.join();

        delete client;
    }

    return 0;
}