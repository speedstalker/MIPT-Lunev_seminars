#ifndef __ERROR_H_INCLUDED
#define __ERROR_H_INCLUDED


extern int my_errno;

typedef enum errno_error_codes
        {
        no_error = 0,

        wrong_args,
        mem_err,

        entry_not_found,

        inconsist_state,

        numb_of_error_codes
        } error_code_t;


// exactly the same as 'perror' func
void my_perror (const char* usr_str);


#endif // __ERROR_H_INCLUDED
