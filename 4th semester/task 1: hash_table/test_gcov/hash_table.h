#ifndef __HASH_TABLE_H_INCLUDED
#define __HASH_TABLE_H_INCLUDED

#include <stdio.h>


typedef  void                hash_key_t;
typedef  void                hash_entry_t;
typedef  struct Hash_table*  hash_table_ptr;
typedef  struct Iterator*    iterator_ptr;

typedef  size_t (*hash_func_t)(const hash_key_t* hash_key, size_t hash_key_size, size_t hash_table_size);

// RETURN VALUE:  0 - if success
//               -1 - otherwise, with my_errno indicating the error
int Hash_table_construct (hash_table_ptr* hash_table, size_t hash_table_size, const hash_func_t hashing_func);
int Hash_table_destruct  (hash_table_ptr* hash_table);

int Hash_table_add_elem    (hash_table_ptr hash_table, const hash_key_t* hash_key, size_t hash_key_size,
                                                       const hash_entry_t* entry_to_add, size_t entry_size);
int Hash_table_delete_elem (iterator_ptr* ptr_iter_to_del_elem);

// find elem in hash_table
iterator_ptr find_elem (const hash_table_ptr hash_table, const hash_key_t* hash_key, size_t hash_key_size);

// RETURN VALUE: iterator_ptr - if success
//               NULL         - otherwise, with my_errno indicating the error
iterator_ptr get_iterator    (const hash_table_ptr hash_table);
int          delete_iterator (iterator_ptr*        usr_iter);
iterator_ptr dup_iterator    (const iterator_ptr   usr_iter);

iterator_ptr move_next (      iterator_ptr usr_iter);
iterator_ptr move_prev (      iterator_ptr usr_iter);
int          is_end    (const iterator_ptr usr_iter);         // -1 - error; 0 - no, not end; 1 - yes, is end

// RETURN VALUE:  pointer to required data - if success
//               -1                        - otherwise, with my_errno indicating the error
// !!! - funcs return pointer to new calloced buffer, that should be freed later
hash_key_t*   get_key   (const iterator_ptr usr_iter);
hash_entry_t* get_entry (const iterator_ptr usr_iter);


#endif // __HASH_TABLE_H_INCLUDED
