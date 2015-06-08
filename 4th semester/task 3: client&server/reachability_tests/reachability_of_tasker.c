#include "reachability_of_tasker.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#include <string.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>

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


//------------------------------------------------------------------------------
// Prover
//------------------------------------------------------------------------------
void* reachability_of_tasker_prover (void* udp_sk_ptr)
{
int udp_sk = *(int*)udp_sk_ptr;

char udp_prove_msg[] = UDP_PROVE_TASKER_REACHABILITY_MSG;
int  numb_of_sent_bytes = 0;

struct sockaddr_in addr = {0};

addr.sin_family      = AF_INET;
addr.sin_port        = htons (PORT);
addr.sin_addr.s_addr = htonl (BROADCAST_IP);
memset (addr.sin_zero, '\0', sizeof (addr.sin_zero));

while (1)
        {
        // printf ("start broadcasting the UDP_PROVE_REACHABILITY_MSG...\n");
        if ((numb_of_sent_bytes = sendto (udp_sk, udp_prove_msg, strlen (udp_prove_msg), 0,
                                          (struct sockaddr*)(&addr), sizeof (addr))) == -1)
                HANDLE_ERROR ("udp sendto");
        // printf ("broadcasted %d bytes: \"%s\"\n\n", numb_of_sent_bytes, udp_prove_msg);

        sleep (TASKER_PROVE_SLEEP_PERIOD);
        }

printf ("Should never get here!\n");
pthread_exit (NULL);
}
//------------------------------------------------------------------------------
// Tester
//------------------------------------------------------------------------------
void* reachability_of_tasker_tester (void* udp_sk_ptr)
{
int udp_sk = *(int*)udp_sk_ptr;
int numb_of_recv_bytes = 0;

char udp_buf[sizeof (UDP_PROVE_TASKER_REACHABILITY_MSG) + 1] = "\0";
struct sockaddr_in tasker_addr = {0};
socklen_t tasker_addr_size = 0;

int ret_val = 0;
int is_first_recv = 0;


if ((ret_val = fcntl (udp_sk, F_SETFL, O_NONBLOCK)) == -1)
        HANDLE_ERROR ("fcntl udp_sk O_NONBLOCK");

while (1)
        {
        // printf ("testing tasker reachability: recvfrom...\n");

        sleep (TASKER_TEST_SLEEP_PERIOD);
        is_first_recv = 1;

        do
                {
                tasker_addr_size = sizeof (tasker_addr);

                numb_of_recv_bytes = recvfrom (udp_sk, udp_buf, sizeof (UDP_PROVE_TASKER_REACHABILITY_MSG), 0,
                                                (struct sockaddr*)(&tasker_addr), &tasker_addr_size);

                // some error
                if ((numb_of_recv_bytes == -1) && (errno != EAGAIN) && (errno != EWOULDBLOCK))
                        HANDLE_ERROR ("recvfrom testing tasker reachability");
                // nothing to read
                else if ((numb_of_recv_bytes == -1) && ((errno == EAGAIN) || (errno == EWOULDBLOCK)))
                        {
                        if (is_first_recv == 1)
                                {
                                printf ("\nLost connection with tasker (proven by tester 1)!\n");
                                printf ("Terminating the program...\n");
                                exit   (EXIT_FAILURE);
                                }
                        else
                                break;
                        }
                // tasker closed the connection
                else if (numb_of_recv_bytes == 0)
                        {
                        printf ("\nLost connection with tasker (proven by tester 2)!\n");
                        printf ("Terminating the program...\n");
                        exit   (EXIT_FAILURE);
                        }

                // udp_buf[numb_of_recv_bytes] = '\0';
                // printf ("packet contains: \"%s\"\n\n", udp_buf);

                is_first_recv = 0;
                }
        while (numb_of_recv_bytes > 0);
        }

printf ("Should never get here!\n");
pthread_exit (NULL);
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Undef error handling macroses
//------------------------------------------------------------------------------
#undef IS_DEBUG
#undef HANDLE_ERROR
#undef HANDLE_ERROR_wL
//------------------------------------------------------------------------------

