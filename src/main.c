#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <protocol/StopAndWait.h>
#include <util/socket_utils.h>
#include <interfaces/packet.h>

Control control;
Status status;

/* This is in charge of the physical -> data -> network link */

void physical_layer_control(){
	/* Physical layer will pass through a Unix Socket to the Data link layer */
	/* Data link layer will pass through a Unix Socket to the Network layer */
	/* Connect to a physical layer -> simulated as we are a socket client */
	/* Connect to a network layer -> simulated as we are a socket client */
	ErrorHandler err;
	int counter;
	while(1){
		while (control.initialised == 0){
			/* Until it is not initialised, try to initilise */
			/* check if there is something at control layer */
			/* Used to put the TRX in master or slave */
			check_control_layer(control.control_fd, &control, &status);
			printf("Going to initialise the link\n");
			err = protocol_establishment_routine(initialise_link, &control, &status);
			if (err == IO_ERROR){
				printf("Error at protocol control: %d\n", err);
				return;
			}
			printf("Control initialised: %d\n", control.initialised);
		}
		/* The link is initialised now! We have connection ;) */
		err = protocol_establishment_routine(check_link_availability, &control, &status);
		if (err == IO_ERROR){
			printf("Error at protocol control: %d\n", err);
			return;
		}
		counter = 0;
		while (control.initialised == 1){
			/* checking control layer is done inside */
			err = StopAndWait(&control, &status);
			if (err != NO_ERROR){
				printf("Error at S&W protocol: %d\n", err);
				return;
			}
			err = protocol_establishment_routine(check_link_availability, &control, &status);
			if (err == IO_ERROR){
				printf("Error at protocol control: %d\n", err);
				return;
			}
		}
	}
}

int protocol_routine(char * sock_data_phy, char * sock_data_net, char * ip, char * sock_control){
	char syscall[256];
	while (1){
		printf("Connect sockets\n");
		control.phy_fd = initialise_client_socket(sock_data_phy);
		if (control.phy_fd == -1){
			perror("Openning phyfd: ");
			exit(-1);
		}
		/* Before connecting the IFACE, set UP the IP */
		sprintf(syscall, "ip tuntap del dev %s mode tun", sock_data_net);
		system(syscall);
		sprintf(syscall, "ip tuntap add dev %s mode tun", sock_data_net);
		system(syscall);
		sprintf(syscall, "ip addr add %s/24 dev %s", ip, sock_data_net);
		system(syscall);
		sprintf(syscall, "ip link set dev %s up", sock_data_net);
		system(syscall);

  		control.net_fd = tun_alloc(sock_data_net);  /* tun interface */
		/*control.net_fd = initialise_server_socket(sock_data_net);*/
		if (control.net_fd == -1){
			perror("Opening netfd: ");
			exit(-1);
		}

		control.control_fd = initialise_server_socket(sock_control);

		control.initialised = 0;
		control.packet_counter = 3;
		control.ping_link_time = 5000;
		control.piggy_time = 20;
		/* init value of packet timeout time */
		control.packet_timeout_time = 500; /* ms */ /* The channel has a delay of 10 ms, so 100 ms per timeout as an example */
		control.round_trip_time = control.packet_timeout_time;
		control.death_link_time = 10000; /* in ms */ /* after 10 seconds without handshake, test again */

		printf("The three socket are initialised\n");

		physical_layer_control();

		close(control.phy_fd);
		close(control.net_fd);
		close(control.control_fd);

	}	
	return 0;
}

int main (int argc, char ** argv) {
	if (argc != 6){
		exit(-1);
	}
	if (strcmp("master", argv[5]) == 0){
		control.master_slave_flag = MASTER;
	}else if (strcmp("slave", argv[5]) == 0){
		control.master_slave_flag = SLAVE;
	}else{
		exit(-1);
	}
	return (protocol_routine(argv[1], argv[2], argv[3], argv[4]));
}


















