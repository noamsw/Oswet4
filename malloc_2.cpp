#include <unistd.h>

struct MallocMetaData
{
    size_t size; // the size of the effective allocation
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

    if (first_node == NULL) // there are no blocks in our list
    {
        // alocating MetaData
        first_node = (MallocMetaData*)sbrk(sizeof(MallocMetaData));
        first_node->size = size;
        first_node->is_free = false;
        first_node->next = NULL;
        first_node->prev = NULL;

        // allocating requested size
        void* block_ptr = sbrk(size);
        if (block_ptr == (void*)(-1))
        {
            // free metadata?
            return NULL;
        }
        return block_ptr;
    }

    // else, there are nodes in the list
    // look for a free block thhat fits
    size_t total_size = size + sizeof(MallocMetaData);
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
        new_meta->size = size;
        new_meta->is_free = false;
        new_meta->next = NULL;
        new_meta->prev = last_node;
        last_node->next = new_meta;

        // alloc requested
        void* block_ptr = sbrk(size);
        if (block_ptr == (void*)(-1))
        {
            // free metadata?
            return NULL;
        }
        return block_ptr;
    }
    else // we found a free node that fits
    {

    }


    void* block_ptr = NULL; //ptr to the allocated block
    if (block_ptr == (void*)(-1))
    {
        return NULL;
    }

    MAllocMetaData* node_to_alloc = first_node;
    // if no node is alocced

    return NULL;
}
