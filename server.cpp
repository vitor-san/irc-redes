#include "socket.hpp"
#include "utils.hpp"
#include <iostream>

using namespace std;

int main() {

    int bytes_in;
    string buff_in, buff_out;

    Socket listener("any", 9001);

    // Open server for, at most, 10 connections
    listener.open_listen(10);

    cout << "Pai tá online.\n"; // Remove later

    Socket *client;
    while (true) {
        client = listener.accept_connection();
        client->receive_message(buff_in);
        client->send_message(buff_in); // echo
        cout << buff_in << endl;
        buff_in.clear();
        delete client;
    }

    return 0;
}