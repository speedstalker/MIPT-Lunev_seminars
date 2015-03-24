// .h part:
int Hash_table_remove_elem (hash_table_ptr hash_table, const hash_key_t* hash_key, size_t hash_key_size);

// .c part:
int Hash_table_remove_elem (hash_table_t* hash_table, const hash_key_t* hash_key, size_t hash_key_size)
{
int idx_for_arr = 0;
list_node_t *list_head = NULL, *list_iter = NULL;
hash_elem_t* hash_elem = NULL;

if ((hash_table && hash_key && hash_key_size) == 0)
        {
        my_errno = wrong_args;
        return -1;
        }

idx_for_arr = hash_table->func (hash_key, hash_key_size, hash_table->size);

list_head = hash_table->idx_arr[idx_for_arr];
if ( (list_head == NULL) || ((list_head->prev == list_head) && (list_head->next == list_head)) )
        {
        my_errno = entry_not_found;
        return -1;
        }

for (list_iter = list_head->next; list_iter != list_head; list_iter = list_iter->next)
        {
        hash_elem = container_of(list_iter, hash_elem_t, list_node);
        if (hash_elem->key_size != hash_key_size)
                continue;
        if (! memcmp (hash_elem->key, hash_key, hash_key_size))
                {
                // if we came here - we have found the elem that we should delete
                _list_del_entry (list_iter);
                #if (IS_DEBUG_ON == 1)
                if (_delete_hash_elem_from_mem (&hash_elem))
                        assert (!"internal error in Hash_table_remove_elem: wrong args passed to _delete... func");
                #else
                _delete_hash_elem_from_mem (&hash_elem);
                #endif
                return 0;
                }
        }

// if we came here - we haven't found the elem that user wants to delete
my_errno = entry_not_found;
return -1;
}
