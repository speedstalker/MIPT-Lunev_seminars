#define I_AM_TASKER

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>

#include <string.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "../reachability_tests/reachability_of_tasker.h"
#include "../reachability_tests/reachability_of_solver.h"

#include "../color_output/color_output.h"


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

#define PORT            1234
#define BROADCAST_IP    0xffffffff

#define UDP_MSG "Is there anybody out there?"

#define BACKLOG          20
#define MAX_SOLVER_NUMB  BACKLOG

#define WAITING_FOR_CONNECTIONS_PERIOD     4 // in sec
#define MAX_WAIT_FOR_NUMB_OF_THREADS_TIME  2 // in sec


//----------
struct accepted_solver_info
        {
        int                tcp_sk;
        struct sockaddr_in addr;
        socklen_t          addr_size;
        int                numb_of_threads;
        };

struct accept_func_arg
        {
        int                          tcp_sk; // socket on which accept is performed
        int*                         numb_of_connected_solvers_ptr;
        struct accepted_solver_info* arr_of_accepted_solvers;
        };

void*  accept_func (void* accept_func_arg);
//----------
struct solver_task
        {
        int your_numb;
        int numb_of_connected_solvers;

        int your_numb_of_thread_to_start_with;
        int total_numb_of_threads;
        };
//----------


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main (int argc, char* argv[])
{
int i = 0;
int ret_val = 0;

int mutex_ret_val = 0;
//----------
int  udp_sk = 0;
long true  = 1;

int  numb_of_sent_bytes = 0;
char udp_msg[]          = UDP_MSG;

struct sockaddr_in addr      = {0};
socklen_t          addr_size = 0;
//----------
int tcp_sk = 0;

fd_set rfds;
struct timeval tv;

int max_tcp_sk        = 0;
int numb_of_ready_fds = 0;
//----------
struct accept_func_arg      accept_func_arg = {0};
int                         numb_of_connected_solvers = 0;
int                         total_numb_of_threads     = 0;
struct accepted_solver_info arr_of_accepted_solvers[MAX_SOLVER_NUMB] = { {0} };

long temp_numb_of_threads = 0;


int arr_of_if_res_received[MAX_SOLVER_NUMB] = {0};
int numb_of_res_received = 0;

struct solver_tester_info   arr_of_solver_testers[MAX_SOLVER_NUMB]   = { {0} };
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
printf (COLOR_BLUE "tcp_sk has been created!\n");
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
printf ("tcp_sk is listening now!\n\n" COLOR_RESET);
//------------------------------------------------------------------------------
// Create an UDP socket
//------------------------------------------------------------------------------
if ((udp_sk = socket (PF_INET, SOCK_DGRAM, 0)) == -1)
        HANDLE_ERROR ("udp_sk socket");
printf (COLOR_BLUE "udp_sk has been created!\n");
//----------
if (setsockopt (udp_sk, SOL_SOCKET, SO_BROADCAST, &true, sizeof (true)) == -1)
        HANDLE_ERROR ("udp_sk setsockopt BROADCAST");
printf ("udp_sk has been set to BROADCAST mode!\n" COLOR_RESET);
//------------------------------------------------------------------------------
// Make a thread to prove reachability of the tasker
//------------------------------------------------------------------------------
pthread_t reachability_of_tasker_prover_thr_id = 0;

if ((ret_val = pthread_create (&reachability_of_tasker_prover_thr_id,
                               NULL,
                               reachability_of_tasker_prover,
                               &udp_sk)))
        {
        printf (COLOR_RED "Error in pthread_create for the tasker reachability: ret_val = %d\n" COLOR_RESET, ret_val);
        exit (EXIT_FAILURE);
        }

printf (COLOR_CYAN "\nTasker reachability prover created!\n\n" COLOR_RESET);
//------------------------------------------------------------------------------
// Broadcast the message (reachability prover will sometimes do it quicker
// but it does not matter, since prover broadcasts from the same socket/addr)
//------------------------------------------------------------------------------
addr.sin_family      = AF_INET;
addr.sin_port        = htons (PORT);
addr.sin_addr.s_addr = htonl (BROADCAST_IP);
memset (addr.sin_zero, '\0', sizeof (addr.sin_zero));

printf (COLOR_BLUE "start broadcasting the UDP_MSG...\n" COLOR_RESET);
if ((numb_of_sent_bytes = sendto (udp_sk, udp_msg, strlen (udp_msg), 0,
                                  (struct sockaddr*)(&addr), sizeof (addr))) == -1)
        HANDLE_ERROR ("udp sendto");
printf (COLOR_CYAN "broadcasted %d bytes: \"%s\"\n\n" COLOR_RESET, numb_of_sent_bytes, udp_msg);
//------------------------------------------------------------------------------
// Make a thread to accept incoming connections
//------------------------------------------------------------------------------
pthread_t      accept_thread_id = 0;

accept_func_arg.tcp_sk                        = tcp_sk;
accept_func_arg.arr_of_accepted_solvers       = arr_of_accepted_solvers;
accept_func_arg.numb_of_connected_solvers_ptr = &numb_of_connected_solvers;
if ((ret_val = pthread_create (&accept_thread_id,
                               NULL,
                               accept_func,
                               &accept_func_arg)))
        {
        printf (COLOR_RED "Error in pthread_create: ret_val = %d\n" COLOR_RESET, ret_val);
        exit (EXIT_FAILURE);
        }
//----------
// Wait for WAITING_FOR_CONNECTIONS_PERIOD sec and cancel accepting thread
sleep (WAITING_FOR_CONNECTIONS_PERIOD);

ret_val = pthread_cancel (accept_thread_id);
if ((ret_val != 0) && (ret_val != ESRCH))
        {
        printf (COLOR_RED "Error in pthread_cancel: ret_val = %d\n" COLOR_RESET, ret_val);
        exit (EXIT_FAILURE);
        }

if ((ret_val = pthread_join (accept_thread_id, NULL)))
        {
        printf (COLOR_RED "Error in pthread_join: ret_val = %d\n" COLOR_RESET, ret_val);
        exit (EXIT_FAILURE);
        }
//----------
if (numb_of_connected_solvers == 0)
        {
        printf (COLOR_RED "There are no solvers available within a current timeout!\n");
        printf ("Terminating program...\n" COLOR_RESET);
        exit   (EXIT_FAILURE);
        }
printf (COLOR_BLUE "Ended accepting incoming connections,\n");
printf ("%d connection%s ha%s been accepted!\n\n" COLOR_RESET, numb_of_connected_solvers,
                                                              (numb_of_connected_solvers == 1) ? ""  : "s",
                                                              (numb_of_connected_solvers == 1) ? "s" : "ve");
//------------------------------------------------------------------------------
// Receive number of threads from each solver
//------------------------------------------------------------------------------
printf (COLOR_BLUE "Waiting for %d solver%s to send %s number of threads:\n" COLOR_RESET, numb_of_connected_solvers,
                                              (numb_of_connected_solvers == 1) ? ""    : "s",
                                              (numb_of_connected_solvers == 1) ? "its" : "their");
numb_of_res_received = 0;


start_of_receiving_numb_of_threads_routine:

max_tcp_sk = 0;
FD_ZERO (&rfds);
for (i = 0; i < numb_of_connected_solvers; i++)
        {
        // correct numb_of_threads should be strictly positive number
        if (arr_of_accepted_solvers[i].numb_of_threads == 0)
                {
                FD_SET (arr_of_accepted_solvers[i].tcp_sk, &rfds);

                if (arr_of_accepted_solvers[i].tcp_sk > max_tcp_sk)
                        max_tcp_sk = arr_of_accepted_solvers[i].tcp_sk;
                }
        }

tv.tv_sec  = MAX_WAIT_FOR_NUMB_OF_THREADS_TIME;
tv.tv_usec = 0;

if ((numb_of_ready_fds = select (max_tcp_sk + 1, &rfds, NULL, NULL, &tv)) == -1)
        HANDLE_ERROR ("select on receiving numb_of_threads of each solver");
else if (numb_of_ready_fds == 0)
        {
        printf (COLOR_RED "Select on receiving numb_of_threads reach timeout!\n");
        printf ("terminating program...\n" COLOR_RESET);
        exit   (EXIT_FAILURE);
        }

for (i = 0; i < numb_of_connected_solvers; i++)
        {
        if (FD_ISSET (arr_of_accepted_solvers[i].tcp_sk, &rfds) != 0)
                {
                numb_of_recv_bytes   = 0;
                temp_numb_of_threads = 0;
                do
                        {
                        ret_val = 0;
                        if ((ret_val = recv (arr_of_accepted_solvers[i].tcp_sk,
                                             &temp_numb_of_threads + numb_of_recv_bytes,
                                             sizeof (temp_numb_of_threads) - numb_of_recv_bytes, 0)) == -1)
                                HANDLE_ERROR ("recv temp_numb_of_threads");
                        if (ret_val == 0)
                                {
                                printf (COLOR_RED "\nError on solver №%d, recv returned 0 after passing the select!\n", i);
                                printf ("terminating the program...\n" COLOR_RESET);
                                exit   (EXIT_FAILURE);
                                }
                        numb_of_recv_bytes += ret_val;
                        }
                while (numb_of_recv_bytes != sizeof (temp_numb_of_threads));

                printf (COLOR_CYAN "received from solver №%d its numb_of_threads = %ld, from addr: %s\n" COLOR_RESET,
                                        i, temp_numb_of_threads, inet_ntoa (arr_of_accepted_solvers[i].addr.sin_addr));

                arr_of_accepted_solvers[i].numb_of_threads = temp_numb_of_threads;

                total_numb_of_threads += temp_numb_of_threads;
                numb_of_res_received += 1; // number of solvers that sent their results

                numb_of_ready_fds -= 1;
                if (numb_of_ready_fds == 0)
                        break; // all, ready for now, results have been read
                }
        }

if (numb_of_res_received != numb_of_connected_solvers)
        goto start_of_receiving_numb_of_threads_routine;

printf (COLOR_BLUE "Tasker received numb_of_threads from all solvers!\n\n" COLOR_RESET);
//------------------------------------------------------------------------------
// Send info about tasks to each solver
//------------------------------------------------------------------------------
printf (COLOR_BLUE "Started sending %d task%s:\n" COLOR_RESET, numb_of_connected_solvers,
                                                              (numb_of_connected_solvers == 1) ? ""  : "s");
solver_task.numb_of_connected_solvers = numb_of_connected_solvers;
solver_task.total_numb_of_threads     = total_numb_of_threads;

solver_task.your_numb_of_thread_to_start_with = 0;
for (i = 0; i < numb_of_connected_solvers; i++)
        {
        solver_task.your_numb = i;
        numb_of_sent_bytes = 0;
        do
                {
                ret_val = 0;
                if ((ret_val = send (arr_of_accepted_solvers[i].tcp_sk,
                                     &solver_task + numb_of_sent_bytes,
                                     sizeof (solver_task) - numb_of_sent_bytes, 0)) == -1)
                        HANDLE_ERROR ("send solver_task");
                numb_of_sent_bytes += ret_val;
                }
        while (numb_of_sent_bytes != sizeof (solver_task));
        printf (COLOR_CYAN "task №%d has been sent to %s\n" COLOR_RESET, i, inet_ntoa (arr_of_accepted_solvers[i].addr.sin_addr));

        solver_task.your_numb_of_thread_to_start_with += arr_of_accepted_solvers[i].numb_of_threads;
        }
printf (COLOR_BLUE "All tasks have been sent!\n\n" COLOR_RESET);
//------------------------------------------------------------------------------
// Make a thread to test reachability of all solvers
//------------------------------------------------------------------------------
// Create and bind UDP sockets to receive messages from solvers
for (i = 0; i < numb_of_connected_solvers; i++)
        {
        if ((arr_of_solver_testers[i].udp_sk = socket (PF_INET, SOCK_DGRAM, 0)) == -1)
                HANDLE_ERROR ("udp_sk socket");
        printf (COLOR_BLUE "udp_sk for testing solver №%d has been created!\n" COLOR_RESET, i);
        //----------
        addr_size = sizeof (addr);
        if (getsockname (arr_of_accepted_solvers[i].tcp_sk, (struct sockaddr*)&addr, &addr_size))
                HANDLE_ERROR ("getsockname tcp_sk");

        addr.sin_family      = AF_INET;
        addr.sin_port        = START_PORT + i;
        addr.sin_addr.s_addr = addr.sin_addr.s_addr;
        memset (addr.sin_zero, '\0', sizeof (addr.sin_zero));

        if (bind (arr_of_solver_testers[i].udp_sk, (struct sockaddr*)(&addr), sizeof (addr)) == -1)
                HANDLE_ERROR ("udp_sk bind");
        // printf ("udp_sk has been binded!\n");

        if ((ret_val = fcntl (arr_of_solver_testers[i].udp_sk, F_SETFL, O_NONBLOCK)) == -1)
                HANDLE_ERROR ("fcntl udp_sk O_NONBLOCK");

        }
//----------
pthread_t reachability_of_solver_tester_thr_id = 0;

struct reach_of_solv_test_arg reach_of_solv_test_arg = {0};
reach_of_solv_test_arg.numb_of_connected_solvers = numb_of_connected_solvers;
reach_of_solv_test_arg.arr_of_solver_testers     = arr_of_solver_testers;
reach_of_solv_test_arg.arr_of_if_res_received    = arr_of_if_res_received;

if ((ret_val = pthread_create (&reachability_of_solver_tester_thr_id,
                               NULL,
                               reachability_of_solver_tester,
                               &reach_of_solv_test_arg)))
        {
        printf (COLOR_RED "Error in pthread_create for solver reachability: ret_val = %d\n" COLOR_RESET, ret_val);
        exit (EXIT_FAILURE);
        }

printf (COLOR_CYAN "\nSolvers reachability tester created!\n\n" COLOR_RESET);
//------------------------------------------------------------------------------
// Receive answers from solvers
//------------------------------------------------------------------------------
printf (COLOR_BLUE "Waiting for %d partial result%s:\n" COLOR_RESET, numb_of_connected_solvers,
                                              (numb_of_connected_solvers == 1) ? ""  : "s");
numb_of_res_received = 0;
general_result = 0;

start_of_receiving_res_routine:

max_tcp_sk = 0;
FD_ZERO (&rfds);
for (i = 0; i < numb_of_connected_solvers; i++)
        {
        if (arr_of_if_res_received[i] == 0)
                {
                FD_SET (arr_of_accepted_solvers[i].tcp_sk, &rfds);

                if (arr_of_accepted_solvers[i].tcp_sk > max_tcp_sk)
                        max_tcp_sk = arr_of_accepted_solvers[i].tcp_sk;
                }
        }

if ((numb_of_ready_fds = select (max_tcp_sk + 1, &rfds, NULL, NULL, NULL)) == -1) // NULL timeout => indefinitely
        HANDLE_ERROR ("select on receiving results");
else if (numb_of_ready_fds == 0)
        assert (!"Should never came here!"); // because of NULL timeout

for (i = 0; i < numb_of_connected_solvers; i++)
        {
        if (FD_ISSET (arr_of_accepted_solvers[i].tcp_sk, &rfds) != 0)
                {
                numb_of_recv_bytes = 0;
                partial_result     = 0;
                do
                        {
                        ret_val = 0;
                        if ((ret_val = recv (arr_of_accepted_solvers[i].tcp_sk,
                                             &partial_result + numb_of_recv_bytes,
                                             sizeof (partial_result) - numb_of_recv_bytes, 0)) == -1)
                                HANDLE_ERROR ("recv partial_result");
                        if (ret_val == 0)
                                {
                                printf (COLOR_RED "\nError on solver №%d, recv returned 0 after passing the select "
                                                                        "on receiving the partial_result!\n", i);
                                printf ("terminating the program...\n" COLOR_RESET);
                                exit   (EXIT_FAILURE);
                                }
                        numb_of_recv_bytes += ret_val;
                        }
                while (numb_of_recv_bytes != sizeof (partial_result));
                printf (COLOR_CYAN "received partial_result = %Lf, from solver №%d, addr: %s\n" COLOR_RESET, partial_result, i,
                                        inet_ntoa (arr_of_accepted_solvers[i].addr.sin_addr));
                //----------
                if ((mutex_ret_val = pthread_mutex_lock (&mutex)) != 0)
                        {
                        printf (COLOR_RED "Error in pthread_mutex_lock while receiving result, ret_val = %d\n" COLOR_RESET,
                                                                                                                mutex_ret_val);
                        exit (EXIT_FAILURE);
                        }

                arr_of_if_res_received[i] = 1;

                if ((mutex_ret_val = pthread_mutex_unlock (&mutex)) != 0)
                        {
                        printf (COLOR_RED "Error in pthread_mutex_unlock while receiving result, ret_val = %d\n" COLOR_RESET,
                                                                                                                mutex_ret_val);
                        exit (EXIT_FAILURE);
                        }
                //----------
                general_result += partial_result;
                numb_of_res_received += 1;

                numb_of_ready_fds -= 1;
                if (numb_of_ready_fds == 0)
                        break; // all, ready for now, results have been read
                }
        }

if (numb_of_res_received != numb_of_connected_solvers)
        goto start_of_receiving_res_routine;

//----------
// We do not need any solvers to be alive now
if ((ret_val = pthread_cancel (reachability_of_solver_tester_thr_id)))
        {
        printf (COLOR_RED "Error in pthread_cancel for solver reachability: ret_val = %d\n" COLOR_RESET, ret_val);
        exit (EXIT_FAILURE);
        }

if ((ret_val = pthread_join (reachability_of_solver_tester_thr_id, NULL)))
        {
        printf (COLOR_RED "Error in pthread_join for solver reachability: ret_val = %d\n" COLOR_RESET, ret_val);
        exit (EXIT_FAILURE);
        }
//----------
printf (COLOR_BLUE "All partial results have been received!\n" COLOR_RESET);

printf (COLOR_CYAN "\nGeneral result = %Lf\n" COLOR_RESET, general_result);
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

//----------
if ((ret_val = pthread_cancel (reachability_of_tasker_prover_thr_id)))
        {
        printf (COLOR_RED "Error in pthread_cancel for the tasker reachability: ret_val = %d\n" COLOR_RESET, ret_val);
        exit (EXIT_FAILURE);
        }

if ((ret_val = pthread_join (reachability_of_tasker_prover_thr_id, NULL)))
        {
        printf (COLOR_RED "Error in pthread_join for the tasker reachability: ret_val = %d\n" COLOR_RESET, ret_val);
        exit (EXIT_FAILURE);
        }
//----------

close (tcp_sk);
close (udp_sk);
//------------------------------------------------------------------------------

return 0;
}


void* accept_func (void* accept_func_arg)
{
int  tcp_sk                                          = ((struct accept_func_arg*)accept_func_arg)->tcp_sk;
int* numb_of_connected_solvers_ptr                   = ((struct accept_func_arg*)accept_func_arg)->numb_of_connected_solvers_ptr;
struct accepted_solver_info* arr_of_accepted_solvers = ((struct accept_func_arg*)accept_func_arg)->arr_of_accepted_solvers;

int i = 0;

struct sockaddr_in solver_addr = {0};
socklen_t          solver_addr_size = 0;

//------------------------------------------------------------------------------
// Accept incoming connections
//------------------------------------------------------------------------------
printf (COLOR_BLUE "Waiting for incoming connections...\n" COLOR_RESET);
for (i = 0; i < MAX_SOLVER_NUMB; i++)
        {
        solver_addr_size = sizeof (solver_addr);
        if ((arr_of_accepted_solvers[i].tcp_sk = accept (tcp_sk, (struct sockaddr*)&solver_addr, &solver_addr_size)) == -1)
                HANDLE_ERROR ("arr_of_accepted_solvers[i].tcp_sk accept");
        arr_of_accepted_solvers[i].addr      = solver_addr;
        arr_of_accepted_solvers[i].addr_size = solver_addr_size;

        *numb_of_connected_solvers_ptr += 1;
        printf (COLOR_CYAN "connection from %s has been accepted!\n" COLOR_RESET, inet_ntoa (solver_addr.sin_addr));
        }
//------------------------------------------------------------------------------

pthread_exit (NULL);
}

