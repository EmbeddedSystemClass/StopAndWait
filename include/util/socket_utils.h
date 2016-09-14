#ifndef __SOCKET_UTILS_H__
#define __SOCKET_UTILS_H__

int initialise_server_socket(char * socket_path);
int initialise_client_socket(char * socket_path);
int tun_alloc(char *dev);

#endif