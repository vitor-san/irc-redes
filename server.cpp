#include "socket.hpp"
#include "utils.hpp"
#include <iostream>
#include <queue>
#include <thread>

using namespace std;

// unordered map -> chave: Socket, dado: sockets_threads
struct sockets_threads {
    thread *send_t;
    thread *receive_t;
};

// TO DO:

// FILA DE MENSAGENS:
//-fila de mensagens a serem enviadas para todos -> msg: identificador de quem mandou, msg em si
//-a thread de cada soquete vai mandar para o client com quem esta conectado todas as mensagens desta fila
//-se uma thread receber do cliente uma mensagem, adiciona a fila de mensagens tal mensagem a ser mandada para todos
//-LOGO, NA INSERCAO NA FILA USAR MUTEX -> REGIAO CRITICA

// THREADS SOCKETS:
//-Cada soquete tera uma thread que recebera as mensagens do canal e enviara se houver alguma mensagem
//-Logo, cada thread tera duas sub threads, uma que espera receber a mensagem do cliente e outra que envia mensagens ao
// cliente (vide o que fizemos no cliente) -vector de threads para o num maximo de cliente
//-thread do listener -> main
// vector de struct que tera ponteiro que thread que envia e thread que recebe

// WARNING: Only send chunks of, at maximum, 2047 chars.
bool send_chunk(string &chunk, Socket *client) {
    // Tries to send the message chunk to the client
    bool success = client->send_message_from(chunk);
    if (!success) { // The server wasn't able to send the message
        return false;
    }
    return true;
}

int main(int argc, const char **argv) {
    uint16_t server_port;
    string in_buff, out_buff; // We need to replicate these for each connection in module 2
    regex r(RGX_CMD);         // RGX_CMD defined in "utils.hpp"
    smatch m;

    // Check if args are valid
    if (argc < 2) {
        fprintf(stderr, "Not enough arguments.\nUsage format: %s {port}\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // Convert the port to an integer
    server_port = atoi(argv[1]);

    // Creates a socket that is only going to listen for incoming connection attempts
    Socket listener("any", server_port);

    // Open server for, at most, 10 connections (not useful for now)
    listener.listening(10);

    Socket *client;
    string cmd;
    string pongMsg("pong"); // aux var
    bool success;

    while (true) {
        // Wait until a new connection arrives. Then, create a new Socket for conversating with this client
        client = listener.accept_connection();
        // Get all messages sent by the client (remember that longer messages will be split in smaller chunks)
        while (client->receive_message_on(in_buff)) {
            // Parses the message, searching for commands
            regex_search(in_buff, m, r);
            cmd = m[1].str(); // Gets first command found, if any
            // If any command where found (following RGX_CMD rules), then execute it
            if (cmd != "") {
                // command "quit" is already handled by the client itself
                if (cmd == "quit") {
                    cout << "Client disconnected" << endl;
                    success = false;
                } else if (cmd == "ping") {
                    success = send_chunk(pongMsg, client);
                }
            } else {
                success = send_chunk(in_buff, client);
            }
            if (!success) {
                cout << "AAAAH" << endl;
                break;
            }
        }

        delete client; // Client quited
    }

    return 0;
}