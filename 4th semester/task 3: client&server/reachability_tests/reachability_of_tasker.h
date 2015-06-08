#ifndef __REACHABILITY_OF_TASKER_H_INCLUDED
#define __REACHABILITY_OF_TASKER_H_INCLUDED


#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>


#define PORT            1234
#define BROADCAST_IP    0xffffffff

#define UDP_PROVE_TASKER_REACHABILITY_MSG       "Tasker is reachable!"

#define TASKER_TEST_SLEEP_PERIOD       5 // in sec
#define TASKER_PROVE_SLEEP_PERIOD      1 // in sec

//------------------------------------------------------------------------------
// Prover
//------------------------------------------------------------------------------
void* reachability_of_tasker_prover (void* udp_sk_ptr);
//------------------------------------------------------------------------------
// Tester
//------------------------------------------------------------------------------
void* reachability_of_tasker_tester (void* udp_sk_ptr);
//------------------------------------------------------------------------------


#endif // __REACHABILITY_OF_TASKER_H_INCLUDED

