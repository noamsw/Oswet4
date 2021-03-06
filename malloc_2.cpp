#include <unistd.h>
#include <cstring>
#include <iostream>
#include "malloc_2.h"

struct MallocMetaData
{
    size_t total_size; // the size of the total allocation
    bool is_free;
    MallocMetaData* next;
    MallocMetaData* prev;
};
MallocMetaData* first_node = NULL; // first node allocated by smalloc

size_t num_free_blocks = 0;
size_t num_free_bytes = 0;
size_t num_allocated_blocks = 0;
size_t num_allocated_bytes = 0;

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
        first_node->total_size = total_size;
        first_node->is_free = false;
        first_node->next = NULL;
        first_node->prev = NULL;
        // std::cout << "size of first alloc: " << first_node->total_size << std::endl;
        // allocating requested size
        void* block_ptr = sbrk(size);
        if (block_ptr == (void*)(-1))
        {
            sbrk((-sizeof(MallocMetaData)));
            first_node = NULL;
            return NULL;
        }
        num_allocated_blocks++;
        num_allocated_bytes += size;
        return block_ptr;
    }

    // else, there are nodes in the list
    // look for a free block thhat fits
    MallocMetaData* curr_node = first_node;
    MallocMetaData* last_node = first_node; // if we wont find a block, update last_node->next
    while (curr_node != NULL)
    {
        if ((curr_node->total_size >= total_size) && curr_node->is_free)
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
        new_meta->total_size = total_size;
        new_meta->is_free = false;
        new_meta->next = NULL;
        new_meta->prev = last_node;
        last_node->next = new_meta;
        // std::cout << "size of sec alloc: " << new_meta->total_size << std::endl;

        // alloc requested
        void* block_ptr = sbrk(size);
        if (block_ptr == (void*)(-1))
        {
            last_node->next = NULL;
            sbrk((-sizeof(MallocMetaData)));
            return NULL;
        }
        num_allocated_blocks++;
        num_allocated_bytes += size;
        return block_ptr;
    }

    else // we found a free node that fits
    {
        curr_node->is_free = false;
        num_free_blocks--;
        num_free_bytes-= (curr_node->total_size - sizeof(MallocMetaData));
        // std::cout << "sizeof MetaData= " << sizeof(MallocMetaData) << std::endl;
        // std::cout << "sum of sizes= " << curr_node+sizeof(MallocMetaData) << std::endl;
        return static_cast<unsigned char*>((void*)(curr_node)) + sizeof(MallocMetaData);
    }
    return NULL;
}

void* scalloc(size_t num, size_t size)
{
    size_t total_size = num*size;
    void* allocated_block = smalloc(total_size);
    if(allocated_block == NULL)
    {
        return NULL;
    }
    memset(allocated_block, 0, total_size);
    return allocated_block;
}

void sfree(void* p)
{
    if(p==NULL)
        return;
    MallocMetaData* curr_meta = (MallocMetaData*)(static_cast<unsigned char*>(p) - sizeof(MallocMetaData));
    // std::cout << "total_size of freed block= " << curr_meta->total_size << std::endl;
    if(curr_meta->is_free)
        return;
    num_free_blocks++;
    num_free_bytes += (curr_meta->total_size - sizeof(MallocMetaData));
    curr_meta->is_free = true;
}

void* srealloc(void* oldp, size_t size)
{
    if (size == 0)
    {
        return NULL;
    }
    if (oldp == NULL)
    {
        return smalloc(size);
    }

    size_t total_size = size + sizeof(MallocMetaData);
    MallocMetaData* curr_meta = (MallocMetaData*)(static_cast<unsigned char*>(oldp) - sizeof(MallocMetaData));
    size_t old_size = curr_meta->total_size -sizeof(MallocMetaData);
    if(total_size <= curr_meta->total_size)
    {
        return oldp;
    }

    void* allocated_block = smalloc(size);
    if(allocated_block == NULL)
    {
        return NULL;
    }
    memcpy(allocated_block, oldp, old_size);
    sfree(oldp);
    return allocated_block;
}

size_t _num_free_blocks()
{
    return num_free_blocks;
    /*
    size_t free_blocks_count = 0;
    MallocMetaData* curr_node = first_node;
    while(curr_node != NULL)
    {
        if(curr_node->is_free)
            free_blocks_count++;
        curr_node = curr_node->next;
    }
    return free_blocks_count;
    */
}

size_t _num_free_bytes()
{
    return num_free_bytes;
    /*
    size_t free_blocks_size = 0;
    MallocMetaData* curr_node = first_node;
    while(curr_node != NULL)
    {
        if(curr_node->is_free)
            free_blocks_size+= (curr_node->total_size - sizeof(MallocMetaData));
        curr_node = curr_node->next;
    }
    return free_blocks_size;
    */
}

size_t _num_allocated_blocks()
{
    return num_allocated_blocks;
}

size_t _num_allocated_bytes()
{
    return num_allocated_bytes;
}

size_t _num_meta_data_bytes()
{
    return num_allocated_blocks*sizeof(MallocMetaData);
}

size_t _size_meta_data()
{
    return sizeof(MallocMetaData);
}

/*
int main()
{
    void* first_block = smalloc(10);
    void* second_block = smalloc(20);
    sfree(first_block);
    void* third_block = smalloc(7);
    void* fourth_block = smalloc(15);
    sfree(third_block);
    std::cout << "allocated blocks = " << _num_allocated_blocks() << std::endl;
    std::cout << "allocated bytes = " << _num_allocated_bytes() << std::endl;
    std::cout << "freed blocks = " << _num_free_blocks() << std::endl;
    std::cout << "freed bytes = " << _num_free_bytes() << std::endl;
}
*/


