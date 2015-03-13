#ifndef __HASH_TABLE_H_INCLUDED
#define __HASH_TABLE_H_INCLUDED

#include <stdio.h>


int my_errno;

typedef  void                hash_key_t;
typedef  void                hash_entry_t;
typedef  struct Hash_table*  hash_table_ptr;
typedef  struct Iterator*    iterator_ptr;

typedef  int (*hash_func)(const hash_key_t*);

// RETURN VALUE:  0 - if success
//               -1 - otherwise, with my_errno indicating the error
int  Hash_table_construct (hash_table_ptr* hash_table, size_t hash_table_size, int (*hash_func)(const hash_key_t*));
void Hash_table_destruct  (hash_table_ptr* hash_table);

int Hash_table_add (hash_table_ptr hash_table, const hash_entry_t*);


// RETURN VALUE:  iterator id - if success
//               -1           - otherwise, with my_errno indicating the error
iterator_ptr get_iterator     (const hash_table_ptr hash_table);
iterator_ptr remove_iterator  (iterator_ptr*        usr_iter);

iterator_ptr move_next (const iterator_ptr usr_iter);
iterator_ptr move_prev (const iterator_ptr usr_iter);
int          is_end    (const iterator_ptr usr_iter);         // 0 - no, not end; 1 - yes, is end

// RETURN VALUE:  id of required data - if success
//               -1                   - otherwise, with my_errno indicating the error
hash_key_t   get_key   (const iterator_ptr usr_iter);
hash_entry_t get_entry (const iterator_ptr usr_iter);

// exactly the same as 'perror' func
void my_perror (const char* usr_str);

#endif // __HASH_TABLE_H_INCLUDED
