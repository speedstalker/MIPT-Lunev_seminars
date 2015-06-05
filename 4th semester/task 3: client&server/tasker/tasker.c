#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#include <string.h>
#include <pthread.h>
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

#define UDP_MSG                                 "Is there anybody out there?"
#define UDP_PROVE_TASKER_REACHABILITY_MSG       "Tasker is reachable!"
#define UDP_PROVE_SOLVER_REACHABILITY_MSG       "Solver is reachable!"

#define BACKLOG          20
#define MAX_SOLVER_NUMB  BACKLOG

#define WAITING_FOR_CONNECTIONS_PERIOD  4 // in sec
#define PROVE_TEST_PERIOD               5 // in sec
#define PROVE_SLEEP_PERIOD              1 // in sec

#define PORT            1234
#define UDP_PORT        1235
#define START_PORT      1236
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

void* reachability_of_tasker_prover (void* udp_sk_ptr);
void* reachability_of_solver_tester (void* reach_of_solv_test_arg_ptr);
struct reach_of_solv_test_arg
        {
        int numb_of_connected_solvers;
        struct solver_tester_info* arr_of_solver_testers;
        };

struct solver_tester_info
        {
        int udp_sk;
        uint16_t tcp_port;
        uint32_t ip;
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
socklen_t addr_size = 0;
//----------
int tcp_sk = 0;
//----------
struct accepted_solver_info arr_of_accepted_solvers[MAX_SOLVER_NUMB] = { {0} };
struct solver_tester_info   arr_of_solver_testers[MAX_SOLVER_NUMB] = { {0} };
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
// Create an UDP socket, bind it (for reachability-check only) and broadcast the message
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
addr.sin_port        = htons (UDP_PORT);
addr.sin_addr.s_addr = htonl (INADDR_ANY);
memset (addr.sin_zero, '\0', sizeof (addr.sin_zero));

// for reachability-check only
if (bind (udp_sk, (struct sockaddr*)(&addr), sizeof (addr)) == -1)
        HANDLE_ERROR ("udp_sk bind");
printf ("udp_sk has been binded!\n");
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
// Make a thread to prove reachability of tasker
//------------------------------------------------------------------------------
pthread_t reachability_of_tasker_prover_thr_id = 0;
pthread_attr_t thread_attr;

if ((ret_val = pthread_attr_init (&thread_attr)))
        {
        printf ("Error in pthread_attr_init for tasker reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }
if ((ret_val = pthread_attr_setdetachstate (&thread_attr, PTHREAD_CREATE_JOINABLE)))
        {
        printf ("Error in pthread_attr_setdetachstate for tasker reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }

if ((ret_val = pthread_create (&reachability_of_tasker_prover_thr_id,
                               &thread_attr,
                               reachability_of_tasker_prover,
                               &udp_sk)))
        {
        printf ("Error in pthread_create for tasker reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }

if ((ret_val = pthread_attr_destroy (&thread_attr)))
        {
        printf ("Error in pthread_attr_destroy for tasker reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }
//------------------------------------------------------------------------------
// Make a thread to accept incoming connections
//------------------------------------------------------------------------------
pthread_t      accept_thread_id = 0;

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
// Wait for WAITING_FOR_CONNECTIONS_PERIOD sec and cancel accepting thread
sleep (WAITING_FOR_CONNECTIONS_PERIOD);

ret_val = pthread_cancel (accept_thread_id);
if ((ret_val != 0) && (ret_val != ESRCH))
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
printf ("Ended accepting incoming connections,\n");
printf ("%d connection%s ha%s been accepted!\n\n", numb_of_connected_solvers,
                                                   (numb_of_connected_solvers == 1) ? ""  : "s",
                                                   (numb_of_connected_solvers == 1) ? "s" : "ve");
//------------------------------------------------------------------------------
// Send info about tasks to each solver
//------------------------------------------------------------------------------
printf ("Started sending %d task%s:\n", numb_of_connected_solvers, (numb_of_connected_solvers == 1) ? ""  : "s");
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
// Make a thread to test reachability of all solvers
//------------------------------------------------------------------------------
for (i = 0; i < numb_of_connected_solvers; i++)
        {
        addr_size = sizeof (addr);
        if (getsockname (arr_of_accepted_solvers[i].tcp_sk, (struct sockaddr*)&addr, &addr_size))
                HANDLE_ERROR ("getsockname tcp_sk");

        arr_of_solver_testers[i].tcp_port = addr.sin_port;
        arr_of_solver_testers[i].ip       = addr.sin_addr.s_addr;

        if ((arr_of_solver_testers[i].udp_sk = socket (PF_INET, SOCK_DGRAM, 0)) == -1)
                HANDLE_ERROR ("udp_sk socket");
        printf ("udp_sk has been created!\n");
        //----------
        addr.sin_family      = AF_INET;
        addr.sin_port        = START_PORT + i; //arr_of_solver_testers[i].tcp_port;
        addr.sin_addr.s_addr = arr_of_solver_testers[i].ip;
        memset (addr.sin_zero, '\0', sizeof (addr.sin_zero));

        // printf ("port: %d, 1234 is %d, 1235 is %d\n", addr.sin_port, htons(1234), htons(1235));
        if (bind (arr_of_solver_testers[i].udp_sk, (struct sockaddr*)(&addr), sizeof (addr)) == -1)
                HANDLE_ERROR ("udp_sk bind");
        // printf ("udp_sk has been binded!\n");

        if ((ret_val = fcntl (arr_of_solver_testers[i].udp_sk, F_SETFL, O_NONBLOCK)) == -1)
                HANDLE_ERROR ("fcntl udp_sk O_NONBLOCK");

        }
//----------
pthread_t reachability_of_solver_tester_thr_id = 0;

if ((ret_val = pthread_attr_init (&thread_attr)))
        {
        printf ("Error in pthread_attr_init for solver reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }
if ((ret_val = pthread_attr_setdetachstate (&thread_attr, PTHREAD_CREATE_JOINABLE)))
        {
        printf ("Error in pthread_attr_setdetachstate for solver reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }

struct reach_of_solv_test_arg reach_of_solv_test_arg = {0};
reach_of_solv_test_arg.numb_of_connected_solvers = numb_of_connected_solvers;
reach_of_solv_test_arg.arr_of_solver_testers     = arr_of_solver_testers;
if ((ret_val = pthread_create (&reachability_of_solver_tester_thr_id,
                               &thread_attr,
                               reachability_of_solver_tester,
                               &reach_of_solv_test_arg)))
                               /*
                               accept_func,
                               &accept_func_arg)))
                               */
        {
        printf ("Error in pthread_create for solver reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }

if ((ret_val = pthread_attr_destroy (&thread_attr)))
        {
        printf ("Error in pthread_attr_destroy for solver reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }
//------------------------------------------------------------------------------
// Receive answers from solvers
//------------------------------------------------------------------------------
printf ("Waiting for %d partial result%s:\n", numb_of_connected_solvers,
                                              (numb_of_connected_solvers == 1) ? ""  : "s");
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
                if (ret_val == 0)
                        {
                        printf ("\nConnection to solver №%d lost!\n", i);
                        printf ("terminating program...\n");
                        exit   (EXIT_FAILURE);
                        }
                numb_of_recv_bytes += ret_val;
                }
        while (numb_of_recv_bytes != sizeof (partial_result));
        printf ("received partial result from solver №%d, addr: %s\n", i, inet_ntoa (arr_of_accepted_solvers[i].addr.sin_addr));
        general_result += partial_result;
        }
//----------
// We do not need any solvers to be alive now
if ((ret_val = pthread_cancel (reachability_of_solver_tester_thr_id)))
        {
        printf ("Error in pthread_cancel for solver reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }

if ((ret_val = pthread_join (reachability_of_solver_tester_thr_id, NULL)))
        {
        printf ("Error in pthread_join for solver reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }
//----------
printf ("All partial results have been received!\n");

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

//----------
if ((ret_val = pthread_cancel (reachability_of_tasker_prover_thr_id)))
        {
        printf ("Error in pthread_cancel for tasker reachability: ret_val = %d\n", ret_val);
        exit (EXIT_FAILURE);
        }

if ((ret_val = pthread_join (reachability_of_tasker_prover_thr_id, NULL)))
        {
        printf ("Error in pthread_join for tasker reachability: ret_val = %d\n", ret_val);
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
struct accepted_solver_info* arr_of_accepted_solvers = ((struct accept_func_arg*)accept_func_arg)->arr_of_accepted_solvers;
int* numb_of_connected_solvers_ptr = ((struct accept_func_arg*)accept_func_arg)->numb_of_connected_solvers_ptr;

int i = 0;

struct sockaddr_in solver_addr = {0};
socklen_t solver_addr_size = 0;

//------------------------------------------------------------------------------
// Accept incoming connections
//------------------------------------------------------------------------------
printf ("Waiting for incoming connections...\n");
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

void* reachability_of_tasker_prover (void* udp_sk_ptr)
{
char udp_prove_msg[] = UDP_PROVE_TASKER_REACHABILITY_MSG;
int numb_of_sent_bytes = 0;

struct sockaddr_in addr = {0};
int udp_sk = *(int*)udp_sk_ptr;

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

        sleep (PROVE_SLEEP_PERIOD);
        }

printf ("Should never get here!\n");
pthread_exit (NULL);
}




void* reachability_of_solver_tester (void* reach_of_solv_test_arg_ptr)
{
int udp_sk = 0;
int numb_of_recv_bytes = 0;

char udp_buf[sizeof (UDP_PROVE_SOLVER_REACHABILITY_MSG) + 1] = "\0";
struct sockaddr_in tasker_addr = {0}, addr = {0};
socklen_t tasker_addr_size = 0;

int i = 0;
int is_first_recv = 0;

int numb_of_connected_solvers =
        ((struct reach_of_solv_test_arg*)reach_of_solv_test_arg_ptr)->numb_of_connected_solvers;
struct solver_tester_info* arr_of_solver_testers =
        ((struct reach_of_solv_test_arg*)reach_of_solv_test_arg_ptr)->arr_of_solver_testers;

while (1)
        {
        sleep (PROVE_TEST_PERIOD);

        for (i = 0; i < numb_of_connected_solvers; i++)
                {
                //printf ("testing solver reachability: recvfrom...\n");

                udp_sk = arr_of_solver_testers[i].udp_sk;
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


                                        addr.sin_family      = AF_INET;
                                        addr.sin_port        = htons (PORT);
                                        addr.sin_addr.s_addr = htonl (BROADCAST_IP);
                                        memset (addr.sin_zero, '\0', sizeof (addr.sin_zero));

                                        if ((sendto (udp_sk, "test", strlen ("test"), 0,
                                                                          (struct sockaddr*)(&addr), sizeof (addr))) == -1)
                                                {
                                                printf ("break!\n");
                                                perror ("sendto");
                                                break;
                                                }



                                        printf ("\nLost connection with solver on port %d (proven by tester 1)!\n", START_PORT + i);
                                        printf ("Terminating the program...\n");
                                        exit   (EXIT_FAILURE);
                                        }
                                else
                                        break;
                                }
                        // solver closed the connection
                        else if (numb_of_recv_bytes == 0)
                                {
                                printf ("NULL!\n");
                                break;
                                /*
                                printf ("\nLost connection with solver (proven by tester 2)!\n");
                                printf ("Terminating the program...\n");
                                exit   (EXIT_FAILURE);
                                */
                                }

                        //udp_buf[numb_of_recv_bytes] = '\0';
                        //printf ("packet contains: \"%s\" from solver on port %d\n\n", udp_buf, START_PORT + i);

                        is_first_recv = 0;
                        }
                while (numb_of_recv_bytes > 0);
                }
        }

printf ("Should never get here!\n");
pthread_exit (NULL);
}


