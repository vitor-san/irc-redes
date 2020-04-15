#ifndef DEFS_H
#define DEFS_H

#include <array>
#include <string>

#define SERVER_PORT 8888
#define BUFF_SIZE 4097
#define MSG_SIZE 4096
#define buffer std::array<char, BUFF_SIZE>

// Returns a new fresh (aka "zeroed") buffer
buffer new_buff();
// Cleans buffer passed by reference
void clean_buff(buffer &b);
// Puts data in buffer. The function automatically cleans the buffer before.
void put_buff(buffer &b, std::string s);

/*
 *   Check for errors. If any, print them to stderr and exit the program.
 *   Parameters:
 *       status (int): status to be checked (only -1 represents an error).
 *       msg (const char array): message to be printed to stderr if an error is
 *   encountered.
 */
void check_error(int status, const char *msg);

#endif