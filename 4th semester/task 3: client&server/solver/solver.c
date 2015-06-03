#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>


//------------------------------------------------------------------------------
// Error handling macroses
//------------------------------------------------------------------------------
#define IS_DEBUG 1

#if (IS_DEBUG == 1)
//{
   #define HANDLE_ERROR HANDLE_ERROR_wL
   #define HANDLE_ERROR_wL(msg)                                                    \
                   do                                                              \
                   {                                                               \
                   char err_msg[256] = {0};                                        \
                                                                                   \
                   snprintf (err_msg, 255, "%d. " msg "%c", __LINE__, '\0');       \
                   perror (err_msg);                                               \
                   exit   (EXIT_FAILURE);                                          \
                   }                                                               \
                   while (0)
//}
#else
//{
   #define HANDLE_ERROR_wL HANDLE_ERROR
   #define HANDLE_ERROR(msg) \
                  do { perror(msg); exit(EXIT_FAILURE); } while (0)
//}
#endif
//------------------------------------------------------------------------------

#define UDP_MSG "Is there anybody out there?"

#define PORT 1234


int main (int argc, char* argv[])
{
char *str = NULL, *endptr = NULL;
long numb_of_threads = 0;

int udp_sk = 0;
char udp_buf[sizeof (UDP_MSG) + 1] = "\0";

int numb_of_recv_bytes = 0;

struct sockaddr_in addr = {0}, tasker_addr = {0};
socklen_t tasker_addr_size = 0;
//----------
int tcp_sk = 0;

//------------------------------------------------------------------------------
// Getting number of threads
//------------------------------------------------------------------------------
if (argc != 2)
        {
        printf ("Usage: %s NUMB_OF_THREADS\n", argv[0]);
        exit (EXIT_FAILURE);
        }

str = argv[1];

errno = 0;
numb_of_threads = strtol (str, &endptr, 10);    // 10 - base of numerical system
if ((errno == ERANGE && (numb_of_threads == LONG_MAX || numb_of_threads == LONG_MIN))
                   || (errno != 0 && numb_of_threads == 0))
        HANDLE_ERROR ("strtol");

if ((endptr == str) || (*endptr != '\0'))
        {
        printf ("NUMB_OF_THREADS must contain only digits!\n");
        exit (EXIT_FAILURE);
        }

if (numb_of_threads <= 0)
        {
        printf ("NUMB_OF_THREADS must be strictly positive number!\n");
        exit (EXIT_FAILURE);
        }
//------------------------------------------------------------------------------
// Create an UDP socket and receive the message
//------------------------------------------------------------------------------
if ((udp_sk = socket (PF_INET, SOCK_DGRAM, 0)) == -1)
        HANDLE_ERROR ("udp_sk socket");
printf ("udp_sk has been created!\n");
//----------
addr.sin_family      = AF_INET;
addr.sin_port        = htons (PORT);
addr.sin_addr.s_addr = htonl (INADDR_ANY);
memset (addr.sin_zero, '\0', sizeof (addr.sin_zero));

if (bind (udp_sk, (struct sockaddr*)(&addr), sizeof (addr)) == -1)
        HANDLE_ERROR ("udp_sk bind");
printf ("udp_sk has been binded!\n\n");
//----------
tasker_addr_size = sizeof (tasker_addr);
printf ("waiting to recvfrom...\n");
if ((numb_of_recv_bytes = recvfrom (udp_sk, udp_buf, sizeof (UDP_MSG), 0,
                                    (struct sockaddr*)(&tasker_addr), &tasker_addr_size)) == -1)
        HANDLE_ERROR ("udp recvfrom");

printf ("got UDP packet from %s\n", inet_ntoa (tasker_addr.sin_addr));
printf ("packet is %d bytes long\n", numb_of_recv_bytes);
udp_buf[numb_of_recv_bytes] = '\0';
printf ("packet contains: \"%s\"\n\n", udp_buf);
//------------------------------------------------------------------------------
// Create a TCP socket and connect to tasker
//------------------------------------------------------------------------------
if ((tcp_sk = socket (PF_INET, SOCK_STREAM, 0)) == -1)
        HANDLE_ERROR ("tcp_sk socket");
printf ("tcp_sk has been created!\n");
//----------
addr.sin_family      = AF_INET;
addr.sin_port        = htons (PORT);
addr.sin_addr.s_addr = tasker_addr.sin_addr.s_addr;
memset (addr.sin_zero, '\0', sizeof (addr.sin_zero));

printf ("trying to connect to %s on port %d\n", inet_ntoa(addr.sin_addr), PORT);
if (connect (tcp_sk, (struct sockaddr*)&addr, sizeof (addr)) == -1)
        HANDLE_ERROR ("tcp_sk connect");
printf ("tcp_sk has been connected to %s!\n\n", inet_ntoa (tasker_addr.sin_addr));
//------------------------------------------------------------------------------


close (tcp_sk);
close (udp_sk);

return 0;
}

