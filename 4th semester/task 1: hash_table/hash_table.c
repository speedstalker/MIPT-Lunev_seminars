#include "hash_table.h"
#include "list.h"
#include "error.h"
#include <assert.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>


//==============================================================================
// Declaring of structures
//==============================================================================
typedef struct Hash_elem hash_elem_t;
struct Hash_elem
        {
        list_node_t   list_node;

        hash_key_t*   key;
        size_t        key_size;
        hash_entry_t* entry;
        size_t        entry_size;
        };

typedef struct Hash_table
        {
        size_t        size;
        hash_func     func;
        list_node_t** idx_arr;
        } hash_table_t;
//==============================================================================


//==============================================================================
// Realisation of functions
//==============================================================================

// RETURN VALUE:  0 - if success
//               -1 - otherwise, with my_errno indicating the error
int Hash_table_construct (hash_table_t** hash_table, size_t hash_table_size, hash_func hashing_func)
{
if ((hash_table && hash_table_size && hashing_func) == 0)
        {
        my_errno = wrong_args;
        return -1;
        }

*hash_table = (hash_table_t*) calloc (1, sizeof (hash_table_t));
if (*hash_table == NULL)
        goto error_table;

(*hash_table)->size = hash_table_size;
(*hash_table)->func = hashing_func;

(*hash_table)->idx_arr = (list_node_t**) calloc (hash_table_size, sizeof (list_node_t*));
if ((*hash_table)->idx_arr == NULL)
        goto error_idx_arr;

if (memset ((*hash_table)->idx_arr, 0, hash_table_size) == NULL)
        goto error_memset;

return 0;


error_memset:
        free ((*hash_table)->idx_arr); (*hash_table)->idx_arr = NULL;
error_idx_arr:
        free (*hash_table); hash_table = NULL;
error_table:
        my_errno = mem_err;
        return -1;
}

// RETURN VALUE:  0 - if success
//               -1 - otherwise, with my_errno indicating the error (if wrong args was given actually)
int Hash_table_destruct (hash_table_t** hash_table)
{
int i = 0;
list_node_t *head = NULL, *curr = NULL ;
hash_elem_t* curr_entry = NULL;

if (hash_table == NULL)
        {
        my_errno = wrong_args;
        return -1;
        }
if (*hash_table == NULL)
        {
        my_errno = wrong_args;
        return -1;
        }

// clear lists of hash_elem_t*
for (i = 0; i < (*hash_table)->size; i++)
        {
        head = (*hash_table)->idx_arr[i];

        if (head == NULL)
                continue;

        while (head->next != head->prev)
                {
                curr = head->next;

                // deleting curr from list
                head->next = curr->next;
                curr->next->prev = head;

                // deleting curr from memory
                curr_entry = container_of (curr, hash_elem_t, list_node);

                free (curr_entry->key  ); curr_entry->key   = NULL;
                free (curr_entry->entry); curr_entry->entry = NULL;
                free (curr_entry);
                }

        head = NULL;
        }

// destruct idx_arr
free ((*hash_table)->idx_arr); (*hash_table)->idx_arr = NULL;

// destruct hash_table
free (*hash_table); *hash_table = NULL;

return 0;
}

// only for internal usage
// RETURN VALUE: hash_elem_t* - if success
//               NULL         - otherwise, with my_errno indicating the error
static hash_elem_t* _create_hash_elem (const hash_key_t* hash_key, size_t hash_key_size,
                       const hash_entry_t* entry_to_add, size_t entry_to_add_size)
{
hash_elem_t* hash_elem = NULL;

if ((hash_key && hash_key_size && entry_to_add && entry_to_add_size) == 0)
        {
        my_errno = wrong_args;
        return NULL;
        }
//-----
hash_elem = (hash_elem_t*) calloc (1, sizeof (hash_elem_t));
if (hash_elem == NULL)
        goto error_hash_elem;

hash_elem->list_node.prev = NULL;
hash_elem->list_node.next = NULL;
//-----
hash_elem->key = (hash_key_t*) calloc (1, hash_key_size);
if (hash_elem->key == NULL)
        goto error_hash_key;

if (memcpy (hash_elem->key, hash_key, hash_key_size) == NULL)
        goto error_memcpy_key;
hash_elem->key_size = hash_key_size;
//-----
hash_elem->entry = (hash_entry_t*) calloc (1, entry_to_add_size);
if (hash_elem->entry == NULL)
        goto error_hash_entry;

if (memcpy (hash_elem->entry, entry_to_add, entry_to_add_size) == NULL)
        goto error_memcpy_entry;
hash_elem->entry_size = entry_to_add_size;
//-----
return hash_elem;


error_memcpy_entry:
        free (hash_elem->entry); hash_elem->entry = NULL;
error_hash_entry:
error_memcpy_key:
        free (hash_elem->key); hash_elem->key = NULL;
error_hash_key:
        free (hash_elem); hash_elem = NULL;
error_hash_elem:
        my_errno = mem_err;
        return NULL;
}

// only for internal usage
// deletes only memory reserved by current hash_elem structure,
//      but do not delete it from hash table properly
// RETURN VALUE:  0 - if success
//               -1 - otherwise, with my_errno indicating the error (if wrong args was given actually)
static int _delete_hash_elem_from_mem (hash_elem_t** hash_elem)
{
hash_elem_t* this = NULL;

if (hash_elem == NULL)
        {
        my_errno = wrong_args;
        return -1;
        }
if (*hash_elem == NULL)
        {
        my_errno = wrong_args;
        return -1;
        }

this = *hash_elem;
free (this->key  ); this->key = NULL;
free (this->entry); this->entry = NULL;
free (this       ); this = NULL;

return 0;
}

int Hash_table_add_elem (hash_table_t* hash_table, const hash_key_t* hash_key, size_t hash_key_size,
                                              const hash_entry_t* entry_to_add, size_t entry_to_add_size)
{
int ret_val = 0;
int idx_for_arr = 0;
hash_elem_t* hash_elem = NULL;
list_node_t* list_head = NULL;

if ((hash_table && hash_key && hash_key_size && entry_to_add && entry_to_add_size) == 0)
        {
        my_errno = wrong_args;
        return -1;
        }

hash_elem = _create_hash_elem (hash_key, hash_key_size, entry_to_add, entry_to_add_size);
if (hash_elem == NULL)
        return -1;      // my_errno was set in _create_hash_elem func

idx_for_arr = hash_table->func (hash_elem->key, hash_elem->key_size, hash_table->size);
// as func return size_t, and 0 - correct array index number - no need to check return value of func

if (hash_table->idx_arr[idx_for_arr] == NULL)       // if there is no head of list
        {
        list_head = (list_node_t*) calloc (1, sizeof (list_node_t));
        if (list_head == NULL)
                goto error_list;

        _list_head_init (list_head);

        hash_table->idx_arr[idx_for_arr] = list_head;
        }

// add to the end of list
_list_add_tail (&(hash_elem->list_node), list_head);

return 0;


error_list:
        ret_val = _delete_hash_elem_from_mem (&hash_elem);
        if (ret_val == -1)
                assert (!"internal error in Hash_table_add: wrong args passed to _delete... func");
        my_errno = mem_err;
        return -1;
}

int Hash_table_remove_elem (hash_table_t* hash_table, const hash_key_t* hash_key, size_t hash_key_size)
{
int idx_for_add = 0;
hash_elem_t* hash_elem = NULL;

if ((hash_table && hash_key && hash_key_size) == 0)
        {
        my_errno = wrong_args;
        return -1;
        }

idx_for_arr = hash_table->func (hash_key, hash_key_size, hash_table->size);
if (hash_table->idx_arr[idx_for_arr] == NULL)
        {
        my_errno = entry_not_found;
        return -1;
        }

while ()



}






//==============================================================================

