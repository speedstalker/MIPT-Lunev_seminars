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
#define BACKLOG 20

#define PORT 1234

int main (int argc, char* argv[])
{
int udp_sk = 0;
long true = 1;

int numb_of_sent_bytes = 0;

char udp_msg[] = UDP_MSG;
struct sockaddr_in addr = {0};
//----------
int tcp_sk = 0;
int solver_tcp_sk = 0;

struct sockaddr_in solver_addr = {0};
socklen_t solver_addr_size = 0;

//------------------------------------------------------------------------------
if (argc != 1)
        {
        printf ("Run the program without arguments\n");
        exit (EXIT_FAILURE);
        }
//------------------------------------------------------------------------------
// Create a TCP socket, bind it and set it to the listen state
//------------------------------------------------------------------------------
if ((tcp_sk = socket (PF_INET, SOCK_STREAM, 0)) == -1)
        HANDLE_ERROR ("tcp_sk socket");
printf ("tcp_sk has been created!\n");
//----------
// to avoid 'already in use' messages
if (setsockopt (tcp_sk, SOL_SOCKET, SO_REUSEADDR, &true, sizeof (true)) == -1)
        HANDLE_ERROR ("tcp_sk setsockopt REUSEADDR");
printf ("tcp_sk has been set to REUSEADDR mode!\n\n");
//----------
addr.sin_family      = AF_INET;
addr.sin_port        = htons (PORT);
addr.sin_addr.s_addr = htonl (INADDR_ANY);
memset (addr.sin_zero, '\0', sizeof (addr.sin_zero));

if (bind (tcp_sk, (struct sockaddr*)(&addr), sizeof (addr)) == -1)
        HANDLE_ERROR ("tcp_sk bind");
printf ("tcp_sk has been binded!\n");
//----------
if (listen (tcp_sk, BACKLOG) == -1)
        HANDLE_ERROR ("tcp_sk listen");
printf ("tcp_sk is listening now!\n\n");
//------------------------------------------------------------------------------
// Create an UDP socket and broadcast the message
//------------------------------------------------------------------------------
if ((udp_sk = socket (PF_INET, SOCK_DGRAM, 0)) == -1)
        HANDLE_ERROR ("udp_sk socket");
printf ("udp_sk has been created!\n");
//----------
if (setsockopt (udp_sk, SOL_SOCKET, SO_BROADCAST, &true, sizeof (true)) == -1)
        HANDLE_ERROR ("udp_sk setsockopt BROADCAST");
printf ("udp_sk has been set to BROADCAST mode!\n\n");
//----------
addr.sin_family      = AF_INET;
addr.sin_port        = htons (PORT);
addr.sin_addr.s_addr = htonl (0xffffffff);
memset (addr.sin_zero, '\0', sizeof (addr.sin_zero));

printf ("start broadcasting the UDP_MSG...\n");
if ((numb_of_sent_bytes = sendto (udp_sk, udp_msg, strlen (udp_msg), 0,
                                  (struct sockaddr*)(&addr), sizeof (addr))) == -1)
        HANDLE_ERROR ("udp sendto");
printf ("broadcasted %d bytes: \"%s\"\n\n", numb_of_sent_bytes, udp_msg);
//------------------------------------------------------------------------------
// Accept incoming connections
//------------------------------------------------------------------------------
solver_addr_size = sizeof (solver_addr);
printf ("start accepting incoming connections...\n");
if ((solver_tcp_sk = accept (tcp_sk, (struct sockaddr*)&solver_addr, &solver_addr_size)) == -1)
        HANDLE_ERROR ("solver_tcp_sk accept");
printf ("connection from %s has been accepted!\n\n", inet_ntoa (solver_addr.sin_addr));
//------------------------------------------------------------------------------



close (tcp_sk);
close (udp_sk);

return 0;
}

