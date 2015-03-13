#include "hash_table.h"
#include <stdio.h>
#include <malloc.h>

int my_errno = 0;

typedef enum errno_error_codes
        {
        no_error = 0,

        wrong_args,
        mem_err,

        numb_of_error_codes
        } error_code_t;

static const char* errno_error_definit[numb_of_error_codes] =
        {
        [no_error] = "no error has occurred",

        [wrong_args] = "wrong arguments was given to the function",
        [mem_err] = "memory error has occured",

        };

typedef struct Hash_elem hash_elem_t;
struct Hash_elem
        {
        hash_key_t*   key;
        hash_entry_t* entry;
        hash_elem_t*  next;
        };

typedef struct Hash_table
        {
        size_t        size;
        hash_func     func;
        hash_elem_t** idx_arr;
        } hash_table_t;


// RETURN VALUE:  0 - if success
//               -1 - otherwise, with my_errno indicating the error
int  Hash_table_construct (hash_table_t** hash_table, size_t hash_table_size, int (*hash_func)(const hash_key_t*))
{
if ((hash_table && hash_table_size && hash_func) == 0)
        {
        my_errno = wrong_args;
        return -1;
        }

*hash_table = (hash_table_t*) calloc (1, sizeof (hash_table_t));
if (*hash_table == NULL)
        {
        my_errno = mem_err;
        return -1;
        }

(*hash_table)->size = hash_table_size;
(*hash_table)->func = hash_func;

(*hash_table)->idx_arr = (hash_elem_t**) calloc (hash_table_size, sizeof (hash_elem_t*));
if ((*hash_table)->idx_arr == NULL)
        {
        free (*hash_table); hash_table = NULL;

        my_errno = mem_err;
        return -1;
        }

return 0;
}

void Hash_table_destruct (hash_table_t** hash_table)
{
int i = 0;
hash_elem_t *curr = NULL, *next = NULL;

if (hash_table == NULL)
        {
        my_errno = wrong_args;
        return;
        }

// clear lists of hash_entry_t*
for (i = 0; i < (*hash_table)->size; i++)
        {
        if ((*hash_table)->idx_arr[i] == NULL)
                continue;

        curr = (*hash_table)->idx_arr[i];
        (*hash_table)->idx_arr[i] = NULL;
        while (curr != NULL)
                {
                next = curr->next;

                free (curr->key  ); curr->key   = NULL;
                free (curr->entry); curr->entry = NULL;
                free (curr);
                curr = next;
                }
        }

// destruct idx_arr
free ((*hash_table)->idx_arr); (*hash_table)->idx_arr = NULL;

// destruct hash_table
free (*hash_table); *hash_table = NULL;
}






// exactly the same as 'perror' func
void my_perror (const char* usr_str)
{
printf ("%s: %s\n", usr_str, errno_error_definit[my_errno]);
}





