#ifndef _NET_H_
#define _NET_H_

#include <time.h>
#include <sys/socket.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

int Block(int sock);
int Unblock(int sock);
int SetupUnixServer(const char *file_name);
int SetupUnixClient(const char *file_name);
int SetupTCPClient(unsigned long ip, int server_port, int client_port);
int SetupTCPServer(unsigned long ip, int server_port);
int SetupUDPSocket(unsigned long ip, int port);

#ifdef __cplusplus
}
#endif

#endif /* _NET_H_ */
