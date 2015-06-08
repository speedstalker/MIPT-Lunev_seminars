#ifndef __REACHABILITY_OF_SOLVER_H_INCLUDED
#define __REACHABILITY_OF_SOLVER_H_INCLUDED


#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>


#define PORT       1234
#define START_PORT 1235

#define UDP_PROVE_SOLVER_REACHABILITY_MSG       "Solver is reachable!"

#define SOLVER_TEST_SLEEP_PERIOD       5 // in sec
#define SOLVER_PROVE_SLEEP_PERIOD      1 // in sec

//------------------------------------------------------------------------------
// Prover
//------------------------------------------------------------------------------
void* reachability_of_solver_prover (void* reach_of_solv_prov_arg_ptr);

struct reach_of_solv_prov_arg
        {
        int                udp_sk;
        int                my_numb;
        struct sockaddr_in tasker_addr;
        };
//------------------------------------------------------------------------------
// Tester
//------------------------------------------------------------------------------
void* reachability_of_solver_tester (void* reach_of_solv_test_arg_ptr);

struct reach_of_solv_test_arg
        {
        int                        numb_of_connected_solvers;
        struct solver_tester_info* arr_of_solver_testers;
        };

struct solver_tester_info
        {
        int      udp_sk;
        uint32_t ip;
        };
//------------------------------------------------------------------------------


#endif // __REACHABILITY_OF_SOLVER_H_INCLUDED

