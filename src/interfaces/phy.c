#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <poll.h>
#include <assert.h>
#include <sys/time.h>

#include <interfaces/phy.h>
#include <protocol/StopAndWait.h>
#include <util/socket_utils.h>
#include <interfaces/packet.h>

static unsigned long long millitime() {
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // caculate milliseconds
    return milliseconds;
}

static void printf_packet (BYTE * buff, int len){
	printf("Length: %d ", len);
	printf("type: %c ", buff[0]);
	if (buff[0] == 'D' || buff[0] == 'A'){
		printf("sn: 0x%02X ", buff[1]);
		printf("rn :0x%02X ", buff[2]);
	}
	if (len > 3){
		printf("seq: %d\n", atoi((char *) &buff[3]));
	}else{
		printf("\n");
	}
}

/* Architecture/system dependant */

/* 	This physical layer is waiting for: 	
		- 255 bytes FIXED SIZE packet (performs FEC with fixed length)
	What we need back is:
		- 1500 bytes MTU + Headers (at this moment 3 bytes): 1503 bytes
	Transmitter -> 
		- Encode 239 bytes payload into a 255 byte RS encoded packet
		- On top of that, add a RS erasure code at packet level 
		- Send N 255 bytes packets towards the sender 
	Receiver ->
		- Receive a 255 bytes packet from the receiver
		- Decode 255 bytes RS packet into a 239 bytes payload
		- Put the decoded packet into RS erasure code receiver
		- When RS decoder returns OK -> Pass the packet to upper layer
*/

int write_ack_to_phy(int fd, Control * c, Status * s){
	BYTE ack [1];
	ack[0] = 0x55;
	s->type = 'A';
	write_packet_to_phy(fd, ack, 1, c, s);
	/* first is the packet type */
	return 0;
}

int write_packet_to_phy(int fd, BYTE * p, int len, Control * c, Status * s){
	/* This creates a packet */
	BYTE tosend [len + 3];
	int finallen = len+3;
	int ret;
	/* 3 is packet overhead */
	memcpy(tosend+3, p, len);
	/* copied to tosend */
	/* first is the packet type */
	tosend[0] = s->type;
	/* Then the actual sequence number */
	tosend[1] = s->sn;
	/* Finally the actual request number */
	tosend[2] = s->rn;
	write(fd, &finallen, sizeof(int));
	ret = write(fd, tosend, finallen);
	printf("Sent packet: ");
	printf_packet(tosend, finallen);
	if (ret == finallen){
		return 0;
	}else{
		return -1;
	}
}

/* If timeout is 0, will be read automatically */
/* Control is passed just for if we need to use it in the future */
/* Status is passed to pump out the received status from their part */
/* This function returns -1 when no packet (timeout/error). Returns 0 when control packet arrived and returns > 0 when data packet */
int read_packet_from_phy(int fd, BYTE * p, int timeout, Control * c, Status * s){
	struct pollfd pfd;
	int len;
	int rv;
	int ret;
	int timeout_count = 0;
	pfd.fd = fd;
	pfd.events = POLLIN;
	BYTE buffer[MTU_SIZE + MTU_OVERHEAD];
	rv = poll(&pfd, 1, timeout);
	if (rv == -1){
		perror("poll");
		return -1;
	}else if (rv == 0){
		printf("Timeout\n");
	}else{
		if (pfd.revents & POLLIN){
			ret = read(fd, &len, sizeof(int));
			if (ret != sizeof(int))
				return -1;
			/* Return size -1 (error) */
			ret = read(fd, buffer, len);
			if (ret > 0){
				while (ret != len){
					ret += read(fd, buffer+ret, len);
				}
			}
			/* Asign types and controls */
			s->type = buffer[0];
			s->sn = buffer[1];
			s->rn = buffer[2];
			printf("Received packet from phy-> ");
			printf_packet(buffer, len);
			memcpy(p, buffer+3, len-3);
			return len - 3;
		}	
		/* packet read! */
	}
	/* Return size 0 */
	return -1;
}