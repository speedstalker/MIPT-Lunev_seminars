#ifndef __HASH_TABLE_H_INCLUDED
#define __HASH_TABLE_H_INCLUDED

#include <stdio.h>


typedef  void                hash_key_t;
typedef  void                hash_entry_t;
typedef  struct Hash_table*  hash_table_ptr;
typedef  struct Iterator*    iterator_ptr;

typedef  size_t (*hash_func)(const hash_key_t* hash_key, size_t hash_key_size, size_t hash_table_size);

// RETURN VALUE:  0 - if success
//               -1 - otherwise, with my_errno indicating the error
int Hash_table_construct (hash_table_ptr* hash_table, size_t hash_table_size, hash_func hashing_func);
int Hash_table_destruct  (hash_table_ptr* hash_table);

int Hash_table_add_elem    (hash_table_ptr hash_table, const hash_key_t* hash_key, size_t hash_key_size,
                                                       const hash_entry_t* entry_to_add, size_t entry_size);
int Hash_table_remove_elem (hash_table_ptr hash_table, const hash_key_t* hash_key, size_t hash_key_size);

//  find! is_elem_in_table
int find ();

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


#endif // __HASH_TABLE_H_INCLUDED
