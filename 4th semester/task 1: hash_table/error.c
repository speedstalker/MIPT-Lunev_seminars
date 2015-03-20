#include "error.h"
#include <stdio.h>

//==============================================================================
// Declaring of structures
//==============================================================================
int my_errno = 0;

static const char* errno_error_definit[numb_of_error_codes + 1] =
        {
        [no_error]        = "no error has occurred",

        [mem_err]         = "memory error has occured",
        [wrong_args]      = "wrong arguments was given to the function",

        [entry_not_found] = "entry with key that you specified is not found",

        [inconsist_state] = "hash table is in inconsistent state, \
                                it is better to delete it",
        };
//==============================================================================

//==============================================================================
// Realisation of functions
//==============================================================================

// exactly the same as 'perror' func
inline void my_perror (const char* usr_str)
{
printf ("%s: %s\n", usr_str, errno_error_definit[my_errno]);
}
//==============================================================================
