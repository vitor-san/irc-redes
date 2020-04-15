#include "defs.h"

buffer new_buff() {
    buffer b;
    clean_buff(b);
    return b;
}

void put_buff(buffer &b, std::string s) {
    clean_buff(b);
    for (auto i = 0; i < s.size(); i++) {
        b[i] = s[i];
    }
    puts(b.data());
}

void clean_buff(buffer &b) { b[0] = '\0'; }

/*
 *   Check for errors. If any, print them to stderr and exit the program.
 *   Parameters:
 *       status (int): status to be checked (only -1 represents an error).
 *       msg (const char array): message to be printed to stderr if an error is
 *   encountered.
 */
void check_error(int status, const char *msg) {
    if (status == -1) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
}