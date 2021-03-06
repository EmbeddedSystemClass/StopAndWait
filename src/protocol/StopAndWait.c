#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <poll.h>
#include <assert.h>
#include <sys/time.h>

#include <protocol/StopAndWait.h>
#include <util/socket_utils.h>
#include <interfaces/packet.h>
#include <interfaces/phy.h>
#include <interfaces/net.h>

static unsigned long long millitime() {
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // caculate milliseconds
    return milliseconds;
}

static ErrorHandler Connect_Master(Control * c, Status * s);
static ErrorHandler Connect_Slave(Control * c, Status * s);

ErrorHandler check_control_layer(int fd, Control * c, Status * s){
	printf("Checking control layer\n");
	printf("Will choose a random number, then select a master and a slave\n");
	if (rand()%100 > 50){
		printf("I will be MASTER!\n");
		c->master_slave_flag = MASTER;
	}else{
		printf("I will be SLAVE!\n");
		c->master_slave_flag = SLAVE;
	}
	return NO_ERROR;
}

ErrorHandler protocol_control_routine (BYTE * p, Control * c, Status * s) {
	ErrorHandler ret;
	/* Routine to send control frames to the other peer */
	/* We shall not be waiting an ACK before sending a control frame */
	if (c->waiting_ack == false){
		/* We can send the control frame */
		s->type = 'P'; /* i.e. P will be a control frame */
		BYTE buffer[MTU_SIZE + MTU_OVERHEAD];
		int len;
		/* put the control information inside */
		buffer[0] = 'A';
		buffer[1] = 'B';
		len = 2;
		if (write_packet_to_phy(c->phy_fd, buffer, len, c, s) != NO_ERROR){
			printf("Error writing\n");
			return IO_ERROR;
		}
		s->stored_count = 0;
		s->stored_type = s->type;
		s->stored_len = len;
		memcpy(s->stored_packet, buffer, len);
		c->waiting_ack = true;
		/* We are waiting an ACK now!! */
		/* Start the timeout */
		c->timeout = millitime();
	}
	return NO_ERROR;
}

ErrorHandler protocol_establishment_routine (ProtocolControlEvent event, Control * c, Status * s){
	ErrorHandler ret;
	switch (event){
		case initialise_link:
			/* Three way handshake, if it is done -> set initialised */
			if (c->master_slave_flag == SLAVE){ /* we are slaves */
				/* Something has been received -> process */
				/* Do Connection */
				ret = Connect_Slave(c, s);
				if (ret == NO_ERROR){
					c->initialised = 1;
					c->waiting_ack = false;
					c->last_link = millitime();
				}else if (ret == IO_ERROR){
					return IO_ERROR;
				}else{
					c->initialised = 0;
				}
			}else if (c->master_slave_flag == MASTER){ /* we are masters */
				ret = Connect_Master(c, s);
				if (ret == NO_ERROR){
					c->initialised = 1;
					c->waiting_ack = false;
					c->last_link = millitime();
				}else if (ret == IO_ERROR){
					return IO_ERROR;
				}else{
					c->initialised = 0;
				}
			}
		break;
		case check_link_availability:
			/* Make a connection test */
			if (c->last_link == 0){
				c->initialised = 0;
			/* now we can implement the ping... */
			}else if ( ((int) (millitime() - c->last_link )) < c->death_link_time){
				if (((int) (millitime() - c->last_link )) >= c->ping_link_time){
					/* Still not death but in ping time */
					/* Make a control frame send */
					if (c->master_slave_flag == MASTER){
						/* Here a control exchange will take place from MASTER -> which is the ground station */
						ret = protocol_control_routine(NULL, c, s);
						if (ret == IO_ERROR){
							printf("Error at protocol control: %d\n", ret);
							return ret;
						}
					}
				}
			}else if ( ((int) (millitime() - c->last_link )) >= c->death_link_time){
				printf("Last link was: %llu. Now we are: %llu\n", c->last_link, millitime());
				printf("link is dead -> set control.initialised to 0, return -1\n");
				c->initialised = 0;
			}else{
				c->initialised = 1;
			}
			/* If death link counter is not out, then proceed */
		break;
		case desconnect_link:
			/* Three way handshake, if it is done -> unset initialised */
		break;
		default:
		break;
	}
	return NO_ERROR;
}

/* The master saids Hanshake HELLO */
static ErrorHandler Connect_Master(Control * c, Status * s){
	BYTE connect_packet[] = "Some Useless Information";
	BYTE recv_buffer[MTU_SIZE + MTU_OVERHEAD];
	Status rs;
	int connect_done = 0;
	int ret;
	int len;
	int retry = 0;
	int limit = (int)(c->death_link_time/c->packet_timeout_time);
	limit += 1;
	while(!connect_done){
		s->type = 'C';
		s->sn = 0;
		s->rn = 0;
		/* The master needs -> Send for a Connect packet, wait for a Connect packet, then send ACK */
		if (write_packet_to_phy(c->phy_fd, connect_packet, sizeof(connect_packet), c, s) != 0){
			printf("Error writing\n");
			return IO_ERROR;
		}
		if (len = read_packet_from_phy(c->phy_fd, recv_buffer, c->packet_timeout_time, c, &rs), len >= 0){
			if (rs.type == 'C'){
				s->sn = rs.rn;
				s->rn = (s->rn + 1)%2;
				c->last_link = millitime();
				write_ack_to_phy(c->phy_fd, c, s);
				connect_done = 1;
			}
		}else{
			retry++;
			if (retry > limit){
				return NO_LINK;
			}
		}
		/* Then is OK, a packet has been exchanged */
	}
	return NO_ERROR;
}

/* The slave waits for a Handshake HELLO */
static ErrorHandler Connect_Slave(Control * c, Status * s){
	int connect_done = 0;
	int connect_established = 0;
	int len;
	Status rs;
	BYTE recv_buffer[MTU_SIZE + MTU_OVERHEAD];
	BYTE connect_packet[] = "Some Useless Information";
	while (!connect_done){
		/* The slave needs -> Wait for a Packet, then Send a Packet back (connect packet) */
		if (len = read_packet_from_phy(c->phy_fd, recv_buffer, c->death_link_time, c, &rs), len >= 0){
			if (rs.type == 'C'){
				s->type = 'C';
				s->sn = rs.sn;
				s->rn = rs.rn;
				s->rn = (s->rn + 1)%2;
				c->last_link = millitime();
				if (write_packet_to_phy(c->phy_fd, connect_packet, sizeof(connect_packet), c, s) != 0){
					printf("Error writing\n");
					return IO_ERROR;
				}
				connect_established = 1;
			}else{
				if (rs.rn != s->sn){
					s->sn = rs.rn;
					connect_done = 1;
					if (rs.type == 'D'){
						printf("Connection from slave (ACK) has been done frome piggybacking packet\n");
						write_to_net(c->net_fd, recv_buffer, len);
						s->rn = (s->rn + 1)%2;
						c->last_link = millitime();
						write_ack_to_phy(c->phy_fd, c, s);
					}
				}
			}
		}else if (connect_established == 1){
			if (write_packet_to_phy(c->phy_fd, connect_packet, sizeof(connect_packet), c, s) != 0){
				printf("Error writing\n");
				return IO_ERROR;
			}	
		}else{
			return NO_LINK;
		}
	}
	return NO_ERROR;
}

ErrorHandler StopAndWait(Control * c, Status * s){
	int len;
	int ret;
	int rv;
	Status rs;
	struct pollfd ufds[3];
	BYTE buffer[MTU_SIZE + MTU_OVERHEAD];
	printf("Print the states: SN: %d RN: %d\n", s->sn, s->rn);
	ufds[0].fd = c->net_fd;
	ufds[0].events = POLLIN; // check for normal data
	ufds[1].fd = c->phy_fd;
	ufds[1].events = POLLIN; // check for normal data
	ufds[2].fd = c->control_fd;
	ufds[2].events = POLLIN;

	if (c->waiting_ack == true){
		printf("Waiting for a packet from PHY -> do not accept from net\n");
		rv = poll(&ufds[1], 2, c->round_trip_time);
	}else{
		printf("Waiting for some packet from NET or PHY\n");
		rv = poll(ufds, 3, c->ping_link_time);
	}
	/* Wait for EVENT */
	if (rv == -1){
		perror("Error waiting for event: ");
		return IO_ERROR;
	}else if(rv == 0){
		/* Resend Frame */
		if (c->waiting_ack == true){
			printf("Timeout waiting for ACK, resending a frame if waiting for ack\n");
			if (++s->stored_count == c->packet_counter){
				printf("Timeout EXPIRED\n");
				/* Last link updated, round trip time must be updated */
				/* Every packet sent c->timeout is set */
				c->round_trip_time = c->packet_timeout_time;
				c->waiting_ack = false;
				return NO_ERROR;
			}
			s->type = s->stored_type;
			/* Care!, maybe is not type D */
			if (write_packet_to_phy(c->phy_fd, s->stored_packet, s->stored_len, c, s) != 0){
				printf("Error writing\n");
				return IO_ERROR;
			}
			c->timeout = millitime();
		}
		return NO_ERROR;
	}else{
		/* Check the timeout */
		if ((ufds[0].revents & POLLIN) && c->waiting_ack == false){
			printf("Something at the NET that can be read\n");
			/* Something in Network Layer -> this has priority when not waiting for ACK */
			/* Do stop and wait SEND */
			if (len = read_packet_from_net(c->net_fd, buffer, 0), len < 0){
				printf("Error reading\n");
				return IO_ERROR;
			}
			if (len > 0){
				if (len > MTU_SIZE){
					printf("Maximum MTU reached\n");
					return IO_ERROR;
				}
				/* in ms */
				s->type = 'D';
				if (write_packet_to_phy(c->phy_fd, buffer, len, c, s) != 0){
					printf("Error writing\n");
					return IO_ERROR;
				}
				s->stored_count = 0;
				s->stored_type = s->type;
				s->stored_len = len;
				memcpy(s->stored_packet, buffer, len);
				c->waiting_ack = true;
				/* Start the timeout */
				c->timeout = millitime();
			}else{
				printf("NET Socket has been closed: %d\n", ret);							
				/* End of socket */
				return IO_ERROR;
			}
		}
		/* Something arrived from the medium!! */
		if (ufds[1].revents & POLLIN){
			printf("Something at the medium that can be read\n");
			/* Something at the physical layer -> this is second priority */
			/* Do stop and wait RECV */
			if (len = read_packet_from_phy(c->phy_fd, buffer, 0, c, &rs), len < 0){
				printf("Error reading\n");
				return IO_ERROR;
			}
			/* Now is time to check wheter is that */
			if (rs.type == 'C' && c->master_slave_flag == SLAVE){
				printf("We are in troubles, asking for reconnect\n");
				printf("Waiting flag was: %d\n", c->waiting_ack);
				c->last_link = 0;
				/* The packet is lost */
				return NO_ERROR;
			}
			/* This means, a packet ACKing the last sent packet has been received (we have to update the sequence number) */
			if (rs.rn != s->sn && c->waiting_ack == true){
				printf("Good packet while waiting for ACK. s->sn updated\n");
				s->sn = rs.rn;
				c->waiting_ack = false;
				/* Last link updated, round trip time must be updated */
				/* Every packet sent c->timeout is set */
				c->round_trip_time = 4 * ((millitime() - c->timeout) + 2 * c->piggy_time);
				printf("Rount trip time updated to: %d\n", c->round_trip_time);
				c->last_link = millitime();
				/* A new packet (sent from other station) has been received while witing for ACK */
				if (rs.sn == s->rn){
					/* Data or Control */
					if (rs.type == 'D' || rs.type == 'P'){
						if (rs.type == 'D'){
							printf("Sending packet towards the network. Received a piggybacking ACK\n");
							check_headers_net(buffer, &len);
							write_to_net(c->net_fd, buffer, len);
						}else if (rs.type == 'P'){
							printf("Control Packet arrived-> ");
							printf("0x%02X 0x%02X\n", buffer[0], buffer[1]);
						}
						/* If we are waiting a packet from network, update s->rn and do not send ACK, send a new packet directly */
						rv = poll(&ufds[0], 1, c->piggy_time);
						if (rv == -1){
							perror("poll inside function: ");
							return IO_ERROR;
						}else if (rv == 0){
							s->rn = (s->rn + 1)%2;
							c->last_link = millitime();
							write_ack_to_phy(c->phy_fd, c, s);
							return NO_ERROR;
						}else{
							s->rn = (s->rn + 1)%2;
							c->last_link = millitime();
							return NO_ERROR;
						}
						/* Prevent from going to rs.sn == s->rn from bottom, since we already entered */
					}
					return NO_ERROR;
				}
				return NO_ERROR;
			}

			/* A new packet (sent from other station) has been received, we were not waiting for ACK or nothing */
			if (rs.sn == s->rn){
				if (c->waiting_ack == false){
					printf("Received sn == rn and waiting_ack == false\n");
					if (rs.type == 'D' || rs.type == 'P'){
						if (rs.type == 'D'){
							printf("Sending packet towards the network\n");
							check_headers_net(buffer, &len);
							write_to_net(c->net_fd, buffer, len);
						}else if (rs.type == 'P'){
							printf("Control Packet arrived-> ");
							printf("0x%02X 0x%02X\n", buffer[0], buffer[1]);
						}
						/* If we are waiting a packet from network, update s->rn and do not send ACK, send a new packet directly */
						rv = poll(&ufds[0], 1, c->piggy_time);
						if (rv == -1){
							perror("poll inside function: ");
							return IO_ERROR;
						}else if (rv == 0){
							s->rn = (s->rn + 1)%2;
							c->last_link = millitime();
							write_ack_to_phy(c->phy_fd, c, s);
							return NO_ERROR;
						}else{
							s->rn = (s->rn + 1)%2;
							c->last_link = millitime();
							return NO_ERROR;
						}
					}
				}else{
					/* A packet received not ACKing my last packet, but I was waiting a packet */
					printf("Received sn == rn and waiting_ack == true\n");
					if (rs.type == 'D' || rs.type == 'P'){
						if (rs.type == 'D'){
							printf("Sending packet towards the network while waiting for ACK\n");
							check_headers_net(buffer, &len);
							write_to_net(c->net_fd, buffer, len);
						}else if (rs.type == 'P'){
							printf("Control Packet arrived-> ");
							printf("0x%02X 0x%02X\n", buffer[0], buffer[1]);
						}
						s->rn = (s->rn + 1)%2;
						c->last_link = millitime();	
						write_ack_to_phy(c->phy_fd, c, s);
						return NO_ERROR;
					}
				}
			}else{
				printf("Out of order packet received, send our RN and SN\n");
				if (rs.type == 'D' || rs.type == 'P' || rs.type == 'C'){
					write_ack_to_phy(c->phy_fd, c, s);
					return NO_ERROR;
				}
			}
			/* This cannot be a corrupted frame */
		}
	}
	if (ufds[2].revents & POLLIN){
		printf("We have an event at control layer\n");
		BYTE command[256];
		ret = read(c->control_fd, command, 256);
		if (ret > 0){
			command[ret] = '\0';
			printf("Command found at control layer: %s\n", command);
		}else{
			return IO_ERROR;
		}
	}
	return NO_ERROR;
}