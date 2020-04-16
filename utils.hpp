#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

#define MSG_SIZE 4097

/*
 *   Check for errors. If any, print them to stderr and exit the program.
 *   Parameters:
 *       status (int): status to be checked (only -1 represents an error).
 *       msg (const char array): message to be printed to stderr if an error is
 *   encountered.
 */
void check_error(int status, int error_num, const char *msg);

#endif