#ifndef __PACKET_H__
#define __PACKET_H__

#include <stdint.h>
#include <time.h>
#include <stdbool.h>

#define MTU_SIZE		1500
#define MTU_OVERHEAD	50
#define PHYSICAL_SIZE	255
#define PHY_OVERHEAD	4

typedef unsigned char BYTE;

typedef enum ProtocolControlEvent{
	initialise_link,
	check_link_availability,
	desconnect_link,
}ProtocolControlEvent;

typedef enum MSFLAG{
	MASTER,
	SLAVE,
}MSFLAG;

typedef struct Control{
	int 					initialised;			/* 0 uninitialised, 1 initialised */
	int 					round_trip_time;		/* in ms */
	int 					packet_counter;			/* counter of retransmit counter */
	int 					piggy_time;				/* this is the piggybacking waiting time */
	int 					ping_link_time;			/* in ms */
	int 					death_link_time;		/* death timeout in ms */
	int 					packet_timeout_time; 	/* start packet timeout in ms */
	MSFLAG 					master_slave_flag;		
	int 					net_fd;
	int 					phy_fd;
	bool 					waiting_ack;			/* true or false */
	unsigned long long 		timeout; 				/* in ms */
	unsigned long long 		last_link;				/* last connection timestamp (epoch in milliseconds) */
}Control;

typedef struct Status{
	uint8_t 	type;		/* packet type */
	uint8_t 	sn;			/* sequence number */
	uint8_t 	rn;			/* request number */
	int 		stored_count;
	BYTE		stored_packet[MTU_SIZE + MTU_OVERHEAD]; /* stored packet to forward if requested */
	int 		stored_len;	/* stored packet len to forward if requested */
	uint8_t 	stored_type;
}Status;

#endif