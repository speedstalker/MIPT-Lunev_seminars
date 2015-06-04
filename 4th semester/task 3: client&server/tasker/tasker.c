#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#include <string.h>
#include <pthread.h>

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

#define UDP_MSG         "Is there anybody out there?"

#define BACKLOG         20
#define MAX_SOLVER_NUMB BACKLOG
#define WAITING_PERIOD  4

#define PORT            1234
#define BROADCAST_IP    0xffffffff


struct accepted_solver_info
        {
        int tcp_sk;

        struct sockaddr_in addr;
        socklen_t          addr_size;
        };

void*  accept_func (void* accept_func_arg);
struct accept_func_arg
        {
        int  tcp_sk;                     // socket on which accept is performed
        struct accepted_solver_info* arr_of_accepted_solvers;
        int* numb_of_connected_solvers_ptr;
        };

struct solver_task
        {
        int your_numb;
        int numb_of_connected_solvers;
        };


int main (int argc, char* argv[])
{
int i = 0;
int ret_val = 0;
//----------
int udp_sk = 0;
long true = 1;

int numb_of_sent_bytes = 0;

char udp_msg[] = UDP_MSG;
struct sockaddr_in addr = {0};
//----------
int tcp_sk = 0;
//----------
struct accepted_solver_info arr_of_accepted_solvers[MAX_SOLVER_NUMB] = { {0} };
int numb_of_connected_solvers = 0;
struct accept_func_arg accept_func_arg = {0};
//----------
struct solver_task solver_task = {0};

long double partial_result = 0;
long double general_result = 0;

int numb_of_recv_bytes = 0;

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
printf ("tcp_sk has been set to REUSEADDR mode!\n");
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
printf ("udp_sk has been set to BROADCAST mode!\n");
//----------
addr.sin_family      = AF_INET;
addr.sin_port        = htons (PORT);
addr.sin_addr.s_addr = htonl (BROADCAST_IP);
memset (addr.sin_zero, '\0', sizeof (addr.sin_zero));

printf ("start broadcasting the UDP_MSG...\n");
if ((numb_of_sent_bytes = sendto (udp_sk, udp_msg, strlen (udp_msg), 0,
                                  (struct sockaddr*)(&addr), sizeof (addr))) == -1)
        HANDLE_ERROR ("udp sendto");
printf ("broadcasted %d bytes: \"%s\"\n\n", numb_of_sent_bytes, udp_msg);
//------------------------------------------------------------------------------
// Make a thread to accept incoming connections
//------------------------------------------------------------------------------
pthread_t      accept_thread_id = 0;
pthread_attr_t thread_attr;

if ((ret_val = pthread_attr_init (&thread_attr)))
        {
        printf ("Error in pthread_attr_init: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }
if ((ret_val = pthread_attr_setdetachstate (&thread_attr, PTHREAD_CREATE_JOINABLE)))
        {
        printf ("Error in pthread_attr_setdetachstate: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }

accept_func_arg.tcp_sk                        = tcp_sk;
accept_func_arg.arr_of_accepted_solvers       = arr_of_accepted_solvers;
accept_func_arg.numb_of_connected_solvers_ptr = &numb_of_connected_solvers;
if ((ret_val = pthread_create (&accept_thread_id,
                               &thread_attr,
                               accept_func,
                               &accept_func_arg)))
        {
        printf ("Error in pthread_create: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }

if ((ret_val = pthread_attr_destroy (&thread_attr)))
        {
        printf ("Error in pthread_attr_destroy: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }
//----------
// Wait for WAITING_PERIOD sec and cancel accepting thread
sleep (WAITING_PERIOD);
if ((ret_val = pthread_cancel (accept_thread_id)))
        {
        printf ("Error in pthread_cancel: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }

if ((ret_val = pthread_join (accept_thread_id, NULL)))
        {
        printf ("Error in pthread_join: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }
//----------
if (numb_of_connected_solvers == 0)
        {
        printf ("There are no solvers available within a current timeout!\n");
        printf ("Terminating program...\n");
        exit   (EXIT_FAILURE);
        }
printf ("\nended accepting incoming connections,\n");
printf ("%d connection%s ha%s been accepted!\n\n", numb_of_connected_solvers,
                                                (numb_of_connected_solvers == 1) ? ""  : "s",
                                                (numb_of_connected_solvers == 1) ? "s" : "ve");
//------------------------------------------------------------------------------
// Send info about tasks to each solver
//------------------------------------------------------------------------------
printf ("started sending %d task%s:\n", numb_of_connected_solvers, (numb_of_connected_solvers == 1) ? ""  : "s");
solver_task.numb_of_connected_solvers = numb_of_connected_solvers;
for (i = 0; i < numb_of_connected_solvers; i++)
        {
        solver_task.your_numb = i;
        numb_of_sent_bytes = 0;
        do
                {
                ret_val = 0;
                if ((ret_val = send (arr_of_accepted_solvers[i].tcp_sk,
                                     &solver_task + numb_of_sent_bytes,
                                     sizeof (struct solver_task) - numb_of_sent_bytes, 0)) == -1)
                        HANDLE_ERROR ("send solver_task");
                numb_of_sent_bytes += ret_val;
                }
        while (numb_of_sent_bytes != sizeof (struct solver_task));
        printf ("task №%d has been sent to %s\n", i, inet_ntoa (arr_of_accepted_solvers[i].addr.sin_addr));
        }
printf ("All tasks have been sent!\n\n");
//------------------------------------------------------------------------------
// Receive answers from solvers
//------------------------------------------------------------------------------
printf ("started receiving partial results:\n");
for (i = 0; i < numb_of_connected_solvers; i++)
        {
        numb_of_recv_bytes = 0;
        do
                {
                ret_val = 0;
                if ((ret_val = recv (arr_of_accepted_solvers[i].tcp_sk,
                                     &partial_result + numb_of_recv_bytes,
                                     sizeof (partial_result) - numb_of_recv_bytes, 0)) == -1)
                        HANDLE_ERROR ("recv partial_result");
                numb_of_recv_bytes += ret_val;
                }
        while (numb_of_recv_bytes != sizeof (partial_result));
        printf ("received partial result from solver №%d, addr: %s\n", i, inet_ntoa (arr_of_accepted_solvers[i].addr.sin_addr));
        general_result += partial_result;
        }

printf ("\nGeneral result = %Lf\n", general_result);
//------------------------------------------------------------------------------







//------------------------------------------------------------------------------
// Cleanup
//------------------------------------------------------------------------------
for (i = 0; i < MAX_SOLVER_NUMB; i++)
        {
        if (arr_of_accepted_solvers[i].tcp_sk == 0)
                break;
        close (arr_of_accepted_solvers[i].tcp_sk);
        }
close (tcp_sk);
close (udp_sk);
//------------------------------------------------------------------------------

return 0;
}

void* accept_func (void* accept_func_arg)
{
int  tcp_sk                                          = ((struct accept_func_arg*)accept_func_arg)->tcp_sk;
struct accepted_solver_info* arr_of_accepted_solvers = ((struct accept_func_arg*)accept_func_arg)->arr_of_accepted_solvers;
int* numb_of_connected_solvers_ptr = ((struct accept_func_arg*)accept_func_arg)->numb_of_connected_solvers_ptr;

int i = 0;

struct sockaddr_in solver_addr = {0};
socklen_t solver_addr_size = 0;

//------------------------------------------------------------------------------
// Accept incoming connections
//------------------------------------------------------------------------------
printf ("started accepting incoming connections...\n");
for (i = 0; i < MAX_SOLVER_NUMB; i++)
        {
        solver_addr_size = sizeof (solver_addr);
        if ((arr_of_accepted_solvers[i].tcp_sk = accept (tcp_sk, (struct sockaddr*)&solver_addr, &solver_addr_size)) == -1)
                HANDLE_ERROR ("arr_of_accepted_solvers[i].tcp_sk accept");
        arr_of_accepted_solvers[i].addr      = solver_addr;
        arr_of_accepted_solvers[i].addr_size = solver_addr_size;

        *numb_of_connected_solvers_ptr += 1;
        printf ("connection from %s has been accepted!\n", inet_ntoa (solver_addr.sin_addr));
        }
//------------------------------------------------------------------------------

pthread_exit (NULL);
}





