#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>

#include <util/socket_utils.h>
#include <interfaces/packet.h>

int input_timeout (int filedes, unsigned int milliseconds){
	fd_set set;
	struct timeval timeout;
	/* Initialize the file descriptor set. */
	FD_ZERO (&set);
	FD_SET (filedes, &set);

	/* Initialize the timeout data structure. */
	timeout.tv_sec = 0;
	timeout.tv_usec = milliseconds;

	/* select returns 0 if timeout, 1 if input available, -1 if error. */
	return (select (FD_SETSIZE, &set, NULL, NULL, &timeout));
}

int main(int argc, char ** argv){
	int len;
	int ret;
	int rv;
	struct pollfd pfd;
	BYTE buffer[MTU_SIZE + MTU_OVERHEAD];
	if (argc != 3){
		exit(-1);
	}
	int to_net = open(argv[1], O_RDWR | O_NONBLOCK);
	int from_net = initialise_client_socket(argv[2]);
	if (strcmp(argv[1], "/dev/tun0") == 0){
		system("ifconfig tun0 192.168.2.2 192.168.2.3 up");
	}else{
		system("ifconfig tun1 192.168.2.3 192.168.2.2 up");
	}
	/* Set up iface */
	printf("Client has been initialised\n");
	while(1){
		if (input_timeout(to_net, 10) > 0) {	
			ret = read(to_net, buffer, MTU_SIZE);
			if (ret > 0){
				printf("Read from upper layers -> Len: %d\n", ret);
				write(from_net, &ret, sizeof(int));
				write(from_net, buffer, ret);
			}
		}
		pfd.fd = from_net;
		pfd.events = POLLIN;
		rv = poll(&pfd, 1, 10);
		if (rv == -1){
			perror("poll");
			return -1;
		}else if (rv > 0){
			ret = read(from_net, &len, sizeof(int));
			if (ret != sizeof(int))
				continue;
			ret = read(from_net, buffer, len);
			if (ret > 0){
				printf("Read from lower layers -> Len: %d\n", len);
				while (ret != len){
					ret += read(from_net, buffer+ret, len);
				}
				write(to_net, buffer, len);
			}
		}
	}
}