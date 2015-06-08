#define I_AM_SOLVER

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#include <string.h>
#include <math.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "../reachability_tests/reachability_of_solver.h"
#include "../reachability_tests/reachability_of_tasker.h"

#include "part_of_simp_int.h"


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

#define PORT 1234

#define UDP_MSG "Is there anybody out there?"

#define MAX_WAIT_FOR_TASK_TIME  10 // in sec


//----------
struct solver_task
        {
        int my_numb;
        int numb_of_connected_solvers;
        };
//----------


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main (int argc, char* argv[])
{
int i = 0;
int ret_val = 0;
//----------
char *str = NULL, *endptr = NULL;
long numb_of_threads = 0;
//----------
int  udp_sk = 0;
char udp_buf[sizeof (UDP_MSG) + 1] = "\0";

int numb_of_recv_bytes = 0;
//----------
struct sockaddr_in addr = {0}, tasker_addr = {0};
socklen_t          tasker_addr_size = 0;
//----------
int                tcp_sk = 0;
struct solver_task my_task = {0};
//----------
general_data_t* general_data = NULL;
long double     result = 0;

int numb_of_sent_bytes = 0;

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
// Create an UDP socket, bind it and receive the message
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
printf ("Waiting to recvfrom...\n");
if ((numb_of_recv_bytes = recvfrom (udp_sk, udp_buf, sizeof (UDP_MSG), 0,
                                    (struct sockaddr*)(&tasker_addr), &tasker_addr_size)) == -1)
        HANDLE_ERROR ("udp recvfrom");

printf ("got UDP packet from %s\n", inet_ntoa (tasker_addr.sin_addr));
printf ("packet is %d bytes long\n", numb_of_recv_bytes);
udp_buf[numb_of_recv_bytes] = '\0';
printf ("packet contains: \"%s\"\n\n", udp_buf);
//------------------------------------------------------------------------------
// Make a thread to test reachability of the tasker
//------------------------------------------------------------------------------
pthread_t reachability_of_tasker_tester_thr_id = 0;

if ((ret_val = pthread_create (&reachability_of_tasker_tester_thr_id,
                               NULL,
                               reachability_of_tasker_tester,
                               &udp_sk)))
        {
        printf ("Error in pthread_create for the tasker reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }
//------------------------------------------------------------------------------
// Create a TCP socket and connect to the tasker
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
// Receive the task from the tasker
//------------------------------------------------------------------------------
printf ("Waiting for the task...\n");
fd_set rfds; FD_ZERO (&rfds);
FD_SET (tcp_sk, &rfds);

struct timeval tv;
tv.tv_sec  = MAX_WAIT_FOR_TASK_TIME;
tv.tv_usec = 0;

if ((ret_val = select (tcp_sk + 1, &rfds, NULL, NULL, &tv)) == -1)
        HANDLE_ERROR ("select on task waiting");
else if (ret_val == 0)
        {
        printf ("\nLost connection with the tasker (proven by select tester)!\n");
        printf ("Terminating the program...\n");
        exit   (EXIT_FAILURE);
        }

numb_of_recv_bytes = 0;
do
        {
        ret_val = 0;
        if ((ret_val = recv (tcp_sk,
                             &my_task + numb_of_recv_bytes,
                             sizeof (struct solver_task) - numb_of_recv_bytes, 0)) == -1)
                HANDLE_ERROR ("recv solver_task");
        if (ret_val == 0)
                {
                printf ("\nConnection to the tasker lost!\n");
                printf ("terminating the program...\n");
                exit   (EXIT_FAILURE);
                }
        numb_of_recv_bytes += ret_val;
        }
while (numb_of_recv_bytes != sizeof (struct solver_task));

printf ("task has been received!\n");
printf ("My number is %d of total %d solver%s.\n\n", my_task.my_numb,
                                                     my_task.numb_of_connected_solvers,
                                                     (my_task.numb_of_connected_solvers == 1) ? "" : "s");
//------------------------------------------------------------------------------
// Make a thread to prove reachability of solver
//------------------------------------------------------------------------------
pthread_t reachability_of_solver_prover_thr_id = 0;

struct reach_of_solv_prov_arg reach_of_solv_prov_arg = {0};
reach_of_solv_prov_arg.udp_sk      = udp_sk;
reach_of_solv_prov_arg.my_numb     = my_task.my_numb;
reach_of_solv_prov_arg.tasker_addr = tasker_addr;

if ((ret_val = pthread_create (&reachability_of_solver_prover_thr_id,
                               NULL,
                               reachability_of_solver_prover,
                               &reach_of_solv_prov_arg)))
        {
        printf ("Error in pthread_create for solver reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Piece of task_2: simp_int.c program, i.e. mathematical algorithm
//------------------------------------------------------------------------------
// actually allocs memory for general_data structure (treat it like a header for an array)
// and an array that have size: typeof(*arr_of_thread_numbs) * numb_of_threads
general_data = (general_data_t*)calloc (1, sizeof (general_data_t) +
                                              sizeof (*(general_data->arr_of_thread_numbs)) * numb_of_threads);
if (general_data == 0)
        HANDLE_ERROR("calloc - creating structure");

general_data->numb_of_threads = numb_of_threads;
general_data->fineness        = FINENESS;
if (my_task.my_numb == 0)
        general_data->from = FROM;
else
        general_data->from = (TO - FROM) / my_task.numb_of_connected_solvers * my_task.my_numb + FROM;

if (my_task.my_numb == (my_task.numb_of_connected_solvers - 1))
        general_data->to = TO;
else
        general_data->to = (TO - FROM) / my_task.numb_of_connected_solvers * (my_task.my_numb + 1) + FROM;

// (general_data + 1) points right behind the structure-header => to the first element of an array
// then we just find out what type do pointer to the array member have
general_data->arr_of_thread_numbs = (typeof (general_data->arr_of_thread_numbs))(general_data + 1);

for (i = 0; i < numb_of_threads; i++)
        general_data->arr_of_thread_numbs[i] = i;
//------------------------------------------------------------------------------
// All thread's routines for actual calculating
//------------------------------------------------------------------------------
pthread_t      thread_ids[numb_of_threads];

for (i = 0; i < numb_of_threads; i++)
        if ((ret_val = pthread_create (&thread_ids[i],
                                       NULL,
                                       simpsons_rule_integral,
                                       (general_data->arr_of_thread_numbs + i))))
                {
                printf ("Error in pthread_create: ret_val = %d\n", ret_val);
                exit (EXIT_FAILURE);
                }
printf ("Started calculating...\n");

for (i = 0; i < numb_of_threads; i++)
        if ((ret_val = pthread_join (thread_ids[i], NULL)))
                {
                printf ("Error in pthread_join: ret_val = %d\n", ret_val);
                exit (EXIT_FAILURE);
                }
printf ("finished calculating!\n");
//------------------------------------------------------------------------------
// if we came here - all threads have computed their parts
//------------------------------------------------------------------------------
for (i = 0; i < numb_of_threads; i++)
        result += general_data->arr_of_thread_numbs[i];

printf ("my result = %Lf\n\n", result);
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Send my result to the tasker
//------------------------------------------------------------------------------
printf ("Started sending the result...\n");
numb_of_sent_bytes = 0;
do
        {
        ret_val = 0;
        if ((ret_val = send (tcp_sk,
                             &result + numb_of_sent_bytes,
                             sizeof (result) - numb_of_sent_bytes, 0)) == -1)
                HANDLE_ERROR ("send result");
        numb_of_sent_bytes += ret_val;
        }
while (numb_of_sent_bytes != sizeof (result));
printf ("result has been sent to the the tasker!\n\n");
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Cleanup
//------------------------------------------------------------------------------
free (general_data); general_data = NULL;

//----------
// Cancel the tasker reachability tester
if ((ret_val = pthread_cancel (reachability_of_tasker_tester_thr_id)))
        {
        printf ("Error in pthread_cancel for the tasker reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }

if ((ret_val = pthread_join (reachability_of_tasker_tester_thr_id, NULL)))
        {
        printf ("Error in pthread_join for the tasker reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }
//----------

close (tcp_sk);
close (udp_sk);

//----------
// Cancel solver reachability prover
ret_val = pthread_cancel (reachability_of_solver_prover_thr_id);
if ((ret_val != 0) && (ret_val != ESRCH))
        {
        printf ("Error in pthread_cancel for solver reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }

if ((ret_val = pthread_join (reachability_of_solver_prover_thr_id, NULL)))
        {
        printf ("Error in pthread_join for solver reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }
//------------------------------------------------------------------------------

return 0;
}

