#ifndef UTILS_HPP
#define UTILS_HPP

#include <fstream>
#include <iostream>
#include <map>
#include <math.h>
#include <regex>
#include <string.h>
#include <string>
#include <utility>
#include <vector>

#define MSG_SIZE 2048                  // Limit of characters for a single message
#define NICK_SIZE 50                   // Limit of characters for a nickname
#define MAX_RET 5                      // Maximum number of retransmissions per client
#define RGX_CMD "^\\s*/(\\w+) ?(\\w*)" // Regex to find command given by user in a message

using server_data = std::pair<std::string, uint16_t>;  // IP and a port
using server_dns = std::map<std::string, server_data>; // Maps a name to a server

/*
 *   Check for errors. If any, print them to stderr and exit the program.
 *   Parameters:
 *       status (int): status to be checked (only -1 represents an error).
 *       msg (const char array): message to be printed to stderr if an error is encountered.
 */
void check_error(int status, int error_num, const char *msg);

/*
 *   Breaks the message into chunks of, at max, MSG_SIZE+1 chars (including '\0').
 *   Parameters:
 *       msg (string): message to be broken in smaller parts (if possible).
 *   Returns:
 *       vector of strings: vector containing all chunks of the
 *   partitionated message.
 */
std::vector<std::string> break_msg(std::string msg);

server_dns get_dns();

#endif