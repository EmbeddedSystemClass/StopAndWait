#ifndef __NET_H__
#define __NET_H__

int write_to_net(int fd, BYTE * p, int len);
int read_packet_from_net(int fd, BYTE * p, int timeout);
int check_headers_net(BYTE * p, int * len);

#endif