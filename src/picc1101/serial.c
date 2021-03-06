/******************************************************************************/
/* PiCC1101  - Radio serial link using CC1101 module and Raspberry-Pi         */
/*                                                                            */
/* Serial definitions                                                         */
/*                                                                            */
/*                      (c) Edouard Griffiths, F4EXB, 2015                    */
/*                                                                            */
/******************************************************************************/

#include <fcntl.h>      // File control definitions
#include <errno.h>      // Error number definitions
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <poll.h>
#include <stdlib.h>

#include "main.h"
#include "serial.h"

/* Have to be rewritten:
    - set_serial_parameters -> this opens the UNIX SOCKET
    - write_serial -> write to socket
    - read_serial -> read non block using POLL
*/

// ------------------------------------------------------------------------------------------------
// Get serial speed
speed_t get_serial_speed(uint32_t speed, uint32_t *speed_n)
// ------------------------------------------------------------------------------------------------
{
    printf("Get serial speed has no sense, now is a UNIX SOCKET\n");
    return B0;
    #if 0
    if (speed >= 460800)
    {
        *speed_n = 460800;
        return B460800;
    }
    else if (speed >= 230400)
    {
        *speed_n = 230400;
        return B230400;
    }
    else if (speed >= 115200)
    {
        *speed_n = 115200;
        return B115200;
    }
    else if (speed >= 57600)
    {
        *speed_n = 57600;
        return B57600;
    }
    else if (speed >= 38400)
    {
        *speed_n = 38400;
        return B38400;
    }
    else if (speed >= 19200)
    {
        *speed_n = 19200;
        return B19200;
    }
    else if (speed >= 9600)
    {
        *speed_n = 9600;
        return B9600;
    }
    else if (speed >= 4800)
    {
        *speed_n = 4800;
        return B4800;
    }
    else if (speed >= 2400)
    {
        *speed_n = 2400;
        return B2400;
    }
    else if (speed >= 1200)
    {
        *speed_n = 1200;
        return B1200;
    }
    else if (speed >= 600)
    {
        *speed_n = 600;
        return B600;
    }
    else if (speed >= 300)
    {
        *speed_n = 300;
        return B300;
    }
    else if (speed >= 134)
    {
        *speed_n = 134;
        return B134;
    }
    else if (speed >= 110)
    {
        *speed_n = 110;
        return B110;
    }
    else if (speed >= 75)
    {
        *speed_n = 75;
        return B75;
    }
    else if (speed >= 50)
    {
        *speed_n = 50;
        return B50;
    }
    else
    {
        *speed_n = 0;
        return B0;
    }
    #endif
}

int openUnixSocket(char * sock_path){
    struct sockaddr_un addr;
    int fd;
    int client_fd;
    if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sock_path, sizeof(addr.sun_path)-1);

    unlink(sock_path);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind error");
        exit(EXIT_FAILURE);
    }

    /** Give permission to the file, so others can access without SUDO in case if needed */

    if (listen(fd, 5) == -1) {
        perror("listen error");
        exit(EXIT_FAILURE);
    }

    if ( (client_fd = accept(fd, NULL, NULL)) == -1) {
        perror("accept error");
        exit(-1);
    }
    return client_fd;
}

void set_serial_parameters(serial_t *serial_parameters, arguments_t *arguments)
{
    /* socket init */
    serial_parameters->sock_fd = openUnixSocket(arguments->serial_device);
}

int write_serial(serial_t *serial_parameters, char *msg, int msglen)
// ------------------------------------------------------------------------------------------------
{
    /* write msglen as int32_t, then write the message */
    int32_t len = (int32_t) msglen;
    #ifdef __USE_SOCAT__
    return write(serial_parameters->sock_fd, msg, len);
    #else
    int ret = write(serial_parameters->sock_fd, &len, sizeof(int32_t));
    if (ret > 0){
        return write(serial_parameters->sock_fd, msg, len);
    }else{
        printf("Error writing\n");
        exit(EXIT_FAILURE);
    }
    #endif
}

// ------------------------------------------------------------------------------------------------
// Read from serial interface
int read_serial(serial_t *serial_parameters, char *buf, int buflen)
// ------------------------------------------------------------------------------------------------
{
    struct pollfd pfd;
    int rv;
    int32_t len;
    int readed;
    pfd.fd = serial_parameters->sock_fd;
    pfd.events = POLLIN;
    rv = poll(&pfd, 1, 10); /* read without timeout */
    if (rv == -1){
        perror("Poll error: ");
        exit(EXIT_FAILURE);
    }else if (rv == 0){
        return 0;
    }else{
        if (pfd.revents & POLLIN){
            #ifdef __USE_SOCAT__
            return read(pfd.fd, buf, buflen);
            #else
            if (read(pfd.fd, &len, sizeof(int32_t)) > 0){
                if (len <= buflen){
                     readed = read(pfd.fd, buf, len);
                     while(readed != len){
                        readed += read(pfd.fd, buf+readed, len-readed);
                     }
                     return len;
                }else{
                    return 0;
                }
            }else{
                return 0;
            }
            #endif
        }
    }
} 

#if 0
// ------------------------------------------------------------------------------------------------
// Init serial interface (TNC)
void set_serial_parameters(serial_t *serial_parameters, arguments_t *arguments)
// ------------------------------------------------------------------------------------------------
{
    serial_parameters->SERIAL_TNC = open(arguments->serial_device, O_RDWR | O_NOCTTY | O_NONBLOCK);

    memset (&serial_parameters->tty, 0, sizeof serial_parameters->tty);

    // Error Handling 
    if ( tcgetattr (serial_parameters->SERIAL_TNC, &serial_parameters->tty ) != 0 ) 
    {
        printf("Error %d from tcgetattr: %s\n", errno, strerror(errno));
    }

    // Save old tty parameters 
    serial_parameters->tty_old = serial_parameters->tty;

    // Set Baud Rate 
    cfsetospeed (&serial_parameters->tty, arguments->serial_speed);
    cfsetispeed (&serial_parameters->tty, arguments->serial_speed);

    // Setting other Port Stuff 
    serial_parameters->tty.c_cflag     &=  ~PARENB;            // Make 8n1
    serial_parameters->tty.c_cflag     &=  ~CSTOPB;
    serial_parameters->tty.c_cflag     &=  ~CSIZE;
    serial_parameters->tty.c_cflag     |=  CS8;

    serial_parameters->tty.c_cflag     &=  ~CRTSCTS;           // no flow control
    serial_parameters->tty.c_cc[VMIN]   =  1;                  // read doesn't block
    serial_parameters->tty.c_cc[VTIME]  =  5;                  // 0.5 seconds read timeout
    serial_parameters->tty.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines

    // Make raw 
    cfmakeraw(&serial_parameters->tty);

    // Flush Port, then applies attributes 
    tcflush(serial_parameters->SERIAL_TNC, TCIFLUSH );

    if ( tcsetattr (serial_parameters->SERIAL_TNC, TCSANOW, &serial_parameters->tty ) != 0) 
    {
        printf("Error %d from tcsetattr: %s\n", errno, strerror(errno));
    }    
}

// ------------------------------------------------------------------------------------------------
// Write to serial interface
int write_serial(serial_t *serial_parameters, char *msg, int msglen)
// ------------------------------------------------------------------------------------------------
{
    return write(serial_parameters->SERIAL_TNC, msg, msglen);
}

// ------------------------------------------------------------------------------------------------
// Read from serial interface
int read_serial(serial_t *serial_parameters, char *buf, int buflen)
// ------------------------------------------------------------------------------------------------
{
    return read(serial_parameters->SERIAL_TNC, buf, buflen);
} 

#endif