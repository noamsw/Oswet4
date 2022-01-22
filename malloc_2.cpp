#include <unistd.h>

struct MallocMetaData
{
    size_t size; // the size of the total allocation
    bool is_free;
    MallocMetaData* next;
    MallocMetaData* prev;
};
MallocMetaData* first_node = NULL; // first node allocated by smalloc(?)

// iterate the list, allocate the first free block that fits
void* smalloc(size_t size)
{
    if (size == 0 || size >= 100000000)
    {
        return NULL;
    }

    size_t total_size = size + sizeof(MallocMetaData);

    if (first_node == NULL) // there are no blocks in our list
    {
        // alocating MetaData
        first_node = (MallocMetaData*)sbrk(sizeof(MallocMetaData));
        if (first_node == (void*)(-1))
        {
            first_node = NULL;
            return NULL;
        }
        first_node->size = total_size;
        first_node->is_free = false;
        first_node->next = NULL;
        first_node->prev = NULL;

        // allocating requested size
        void* block_ptr = sbrk(size);
        if (block_ptr == (void*)(-1))
        {
            sbrk((-sizeof(MallocMetaData)));
            first_node = NULL;
            return NULL;
        }
        return block_ptr;
    }

    // else, there are nodes in the list
    // look for a free block thhat fits
    MallocMetaData* curr_node = first_node;
    MallocMetaData* last_node = first_node; // if we wont find a block, update last_node->next
    while (curr_node != NULL)
    {
        if ((curr_node->size >= total_size) && curr_node->is_free)
            break;
        last_node = curr_node;
        curr_node = curr_node->next;
    }

    if (curr_node == NULL) // we did not find a block
    {
        // insert MetaData
        MallocMetaData* new_meta = (MallocMetaData*)sbrk(sizeof(MallocMetaData));
        if (new_meta == (void*)(-1))
        {
            return NULL;
        }
        new_meta->size = total_size;
        new_meta->is_free = false;
        new_meta->next = NULL;
        new_meta->prev = last_node;
        last_node->next = new_meta;

        // alloc requested
        void* block_ptr = sbrk(size);
        if (block_ptr == (void*)(-1))
        {
            last_node->next = NULL;
            sbrk((-sizeof(new_meta)));
            return NULL;
        }
        return block_ptr;
    }

    else // we found a free node that fits
    {
        curr_node->is_free = false;
        return curr_node+sizeof(MallocMetaData);
    }
    return NULL;
}
