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
        exit(EXIT_FAILURE);
    }
}

std::vector<char[MSG_SIZE + 1]> break_msg(std::string msg) {
    int msg_len = msg.size();
    if (msg[msg_len - 1] == '\0')
        msg_len--;

    int n_chunks = int(ceil((double)msg_len / MSG_SIZE + 1)); // -1 to make '\0' fit in the end
    std::vector<char[MSG_SIZE + 1]> chunks(n_chunks);

    int start, chunk_len;
    for (int i = 0; i < n_chunks; i++) {
        start = i * MSG_SIZE + 1;

        if (i + 1 == n_chunks) // Last chunck
            chunk_len = msg_len - start;
        else
            chunk_len = MSG_SIZE + 1;

        msg.copy(chunks[i], chunk_len, start);
        chunks[i][chunk_len] = '\0';
    }

    return chunks;
}
