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
    MallocMetaData* next_bin_list; //the next  in the bin list
    MallocMetaData* prev_bin_list;  //the prev in the bin list
};
MallocMetaData* first_node = NULL; // first node allocated by smalloc
MallocMetaData* last_aloc_node = NULL; //last node allocated by smalloc
MallocMetaData* bin[128];   //bin of free blocks lists
bool init = false;

size_t num_free_blocks = 0;
size_t num_free_bytes = 0;
size_t num_allocated_blocks = 0;
size_t num_allocated_bytes = 0;
void _insertNodeToBin(MallocMetaData* node){
    size_t index = node->total_size/1024;
    if(bin[index]){
        MallocMetaData* curr_node = bin[index];
        while(curr_node){
            if(curr_node->total_size >= node->total_size){
                if(curr_node->prev_bin_list){
                    curr_node->prev_bin_list->next_bin_list = node;
                    node->prev_bin_list = curr_node->prev_bin_list;
                    curr_node->prev_bin_list = node;
                }
                else{
                    curr_node->prev_bin_list = node;
                    bin[index] = node;
                    node->prev_bin_list = NULL;
                }
                node->next_bin_list = curr_node;
                return;
            }
        }
    }
    bin[index] = node;
    node->prev_bin_list = NULL;
}
void _removeNodeFromBin(MallocMetaData* node){
    size_t block_index  = node->total_size/1024;
    if(node==bin[block_index]) {
        if (node->next_bin_list) {
            bin[block_index] = node->prev_bin_list;
            bin[block_index]->prev_bin_list = NULL;
        }
        else{
            bin[block_index] = NULL;
        }
    }
    else{
        if (node->next_bin_list) {
            node->next_bin_list->prev_bin_list = node->prev_bin_list;
        }
        node->prev_bin_list->next_bin_list = node->next_bin_list;
    }
    node->is_free = false;
    node->prev_bin_list = NULL;
    node->next_bin_list = NULL;
}
MallocMetaData* checkAndUpdateBin(size_t size){
    size_t total_size = size + sizeof(MallocMetaData);
    size_t block_index = -1;
    //check if there is a free block of appropiate size in the list at out initial index
    size_t init_index = total_size/1024;
    MallocMetaData* curr_node = NULL;
    if(bin[init_index]){
        MallocMetaData* curr_node = bin[init_index];
        while (curr_node != NULL) {
            if (curr_node->total_size >= total_size){
                block_index = init_index;
                break;
            }
            curr_node = curr_node->next;
        }
    }
    if(!curr_node){
        for(size_t index = init_index+1; index<128; index++){
            if(bin[index]){
                curr_node = bin[index];
                block_index = index;
                break;
            }
        }
    }
    if(curr_node){
        _removeNodeFromBin(curr_node);
        return curr_node;
    }
    return nullptr;
}
void* smalloc(size_t size)
{
    if(!init){//initialize the freeblock array
        init = true;
        for (int i = 0; i<128; i++){
            bin[i] = NULL;
        }
    }
    if (size == 0 || size >= 100000000)//check that size is valid
    {
        return NULL;
    }
    MallocMetaData* curr_node = checkAndUpdateBin(size);
    if(!curr_node) // there are no free blocks of appropriate size in our bin
    {
        MallocMetaData* new_node = (MallocMetaData*)sbrk(sizeof(MallocMetaData));
        if (new_node == (void*)(-1))
        {
            new_node = NULL;
            return NULL;
        }
        new_node->total_size = total_size;
        new_node->is_free = false;
        new_node->next = NULL;
        new_node->prev = NULL;
        new_node->next_bin_list = NULL;
        new_node->prev_bin_list = NULL;
        // std::cout << "size of first alloc: " << first_node->total_size << std::endl;
        // allocating requested size
        void* block_ptr = sbrk(size);
        if (block_ptr == (void*)(-1))
        {
            sbrk((-sizeof(MallocMetaData)));
            new_node = NULL;
            return NULL;
        }
        if(!first_node){    //check if we allocated a block yet, if not set the first and last block
            first_node = new_node;
            last_aloc_node = new_node;
        }
        else{   //if yes, set the next of last to be the new_node, and update the last node
            last_aloc_node->next = new_node;
            new_node->prev = last_aloc_node;
            last_aloc_node = new_node;
        }
        num_allocated_blocks++;
        num_allocated_bytes += size;
        return block_ptr;
    }
    else //there may be a free node of appropriate size
    {

        num_free_blocks--;
        num_free_bytes-= (curr_node->total_size - sizeof(MallocMetaData));
        // std::cout << "sizeof MetaData= " << sizeof(MallocMetaData) << std::endl;
        // std::cout << "sum of sizes= " << curr_node+sizeof(MallocMetaData) << std::endl;
        return static_cast<unsigned char*>((void*)(curr_node)) + sizeof(MallocMetaData);
    }
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
    MallocMetaData *next = curr_meta->next;
    if(next){
        if(next->is_free){
            _removeNodeFromBin(next);
            num_free_blocks--;
            curr_meta->next = next->next;
            curr_meta->total_size += next->total_size;
            if(curr_meta->next){    //we already updated next to be the next of next
                curr_meta->next->prev = curr_meta;
            }
            else{
                last_aloc_node = curr_meta;
            }
        }
    }
    MallocMetaData *prev = curr_meta->prev;
    if(prev){
        if(prev->is_free){
            _removeNodeFromBin(prev);
            num_free_blocks--;
            prev->next = curr_meta->next;
            prev->total_size += curr_meta->total_size;
            if(prev->next){    //we already updated next to be the next of curr
                prev->next->prev = prev;
            }
            else{
                last_aloc_node = prev;
            }
            curr_meta = prev;
        }
    }

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




