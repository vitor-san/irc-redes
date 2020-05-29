#include "utils.hpp"
#include <iostream>

/*
 *   Check for errors. If any, print them to stderr and exit the program.
 *   Parameters:
 *       status (int): status to be checked (only -1 represents an error).
 *       msg (const char array): message to be printed to stderr if an error is encountered.
 */
void check_error(int status, int error_num, const char *msg) {
    if (status == error_num) {
        perror(msg);
    }
}

/*
 *   Breaks the message into chunks of, at max, MSG_SIZE+1 chars (including '\0').
 *   Parameters:
 *       msg (string): message to be broken in smaller parts (if possible).
 *   Returns:
 *       vector<char[MSG_SIZE+1]>: vector containing all chunks of the
 *   partitionated message.
 */
std::vector<std::string> break_msg(std::string msg) {

    int n_chunks = int(ceil((double)msg.size() / MSG_SIZE));
    std::vector<std::string> chunks(n_chunks);

    int j, start_submsg = 0;
    for (int i = 0; i < n_chunks; i++) {
        for (j = start_submsg; j < (start_submsg + MSG_SIZE); j++) {
            chunks[i] += msg[j];
        }
        chunks[i] += '\0';
        start_submsg = j;
    }

    return chunks;
}