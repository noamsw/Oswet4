#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sys/mman.h>
#include "malloc_3.h"
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
MallocMetaData* last_aloc_node = NULL; //last node allocated by smalloc, used as wilderness block
MallocMetaData* bin[128];   //bin of free blocks lists
bool init = false; //to initialize the bin to null
size_t num_free_blocks = 0;
size_t num_free_bytes = 0;
size_t num_allocated_blocks = 0;
size_t num_allocated_bytes = 0;
void _insertNodeToBin(MallocMetaData* node){    //gets a node that wasnt free, and frees it and inserts to bin
    num_free_blocks++;  //in charge of incrementing counters
    num_free_bytes += (node->total_size - sizeof(MallocMetaData));
    node->is_free = true;
    size_t index = (node->total_size - sizeof(MallocMetaData))/1024;
    if(bin[index]){ // the list is not empty
        MallocMetaData* curr_node = bin[index];
        bin[index] = node;
        node->next_bin_list = curr_node;
        curr_node->prev_bin_list = node;
        node->prev_bin_list = NULL;
        return;
        /*lets do a simple implementation first
        MallocMetaData* last_node = bin[index];
        while(curr_node){
            if(curr_node->total_size >= node->total_size){
                if(curr_node->prev_bin_list){ // were not the first node in the list
                    curr_node->prev_bin_list->next_bin_list = node;
                    node->prev_bin_list = curr_node->prev_bin_list;
                    curr_node->prev_bin_list = node;
                }
                else{ // we are the first node
                    curr_node->prev_bin_list = node;
                    bin[index] = node;
                    node->prev_bin_list = NULL;
                }
                node->next_bin_list = curr_node;
                return;
            }
            last_node = curr_node;
            curr_node = curr_node->next_bin_list;
        }
        // were the last node
        last_node->next_bin_list = node;
        node->prev_bin_list = last_node;
        return;
         */
    }
    // the list is empty
    bin[index] = node;
    node->next_bin_list = NULL;
    node->prev_bin_list = NULL;
}

void _removeNodeFromBin(MallocMetaData* node){  //removes a block from the bin, updates counters
    num_free_blocks--;
    num_free_bytes-= (node->total_size - sizeof(MallocMetaData));
    size_t block_index  = (node->total_size - sizeof(MallocMetaData))/1024;
    if(node==bin[block_index]) {    //if its the first node in the list
        if (node->next_bin_list) {  // if there is more than node in the list
            bin[block_index] = node->next_bin_list;
            bin[block_index]->prev_bin_list = NULL;
        }
        else{
            bin[block_index] = NULL;
        }
    }
    else{   //node is not the first block in the bin
        if (node->next_bin_list) {  //if node is not the last, update the previous of its next
            node->next_bin_list->prev_bin_list = node->prev_bin_list;
        }
        node->prev_bin_list->next_bin_list = node->next_bin_list;
    }
    node->is_free = false;
    node->prev_bin_list = NULL;
    node->next_bin_list = NULL;
}

void _splitBlock(MallocMetaData* node, size_t size){    //splits a block into two, the original node stays valid, only smaller
    if((node->total_size - size) < (128 + 2*sizeof(MallocMetaData))){
        return;
    }
    MallocMetaData* second_block = (MallocMetaData*)(static_cast<unsigned char*>((void*)(node)) + sizeof(MallocMetaData) + size);
    second_block->prev_bin_list=NULL;
    second_block->next_bin_list=NULL;
    second_block->total_size = node->total_size - size - sizeof(MallocMetaData);
    node->total_size = size + sizeof(MallocMetaData);
    second_block->next = node->next;
    if(!second_block->next){
        last_aloc_node = second_block;
    }
    else{
        second_block->next->prev = second_block;
    }
    node->next = second_block;
    second_block->prev = node;
    num_allocated_bytes -= sizeof(MallocMetaData);
    num_allocated_blocks++;
    if(second_block->next->is_free){
        _removeNodeFromBin(second_block->next);
        _mergeBlock(second_block, second_block->next);
    }
    _insertNodeToBin(second_block);
}

MallocMetaData* _mergeBlock(MallocMetaData* low_node, MallocMetaData* high_node){ //recieves two adjacent blocks that will be merged
    num_allocated_blocks--; // not sure yet
    num_allocated_bytes += sizeof(MallocMetaData);
    low_node->next = high_node->next;   //they are free, but not in the bin
    low_node->total_size += high_node->total_size;
    MallocMetaData* new_next = low_node->next; // we already updated next to be the next of next
    if(new_next){  // check if the new merged node is the last
        new_next->prev = low_node;
    }
    else{
        last_aloc_node = low_node;
    }
    unsigned char * dest = static_cast<unsigned char*>((void*)low_node)+sizeof(MallocMetaData);
    unsigned char * src = static_cast<unsigned char*>((void*)high_node)+sizeof(MallocMetaData);
    unsigned long cpysize = high_node->total_size- sizeof(MallocMetaData);
    memcpy(dest ,src, cpysize);
    return low_node;
}

MallocMetaData* _superMerge(MallocMetaData* curr_meta, size_t size){  //will attemp to merge a block with its adjacent blocks
    if(curr_meta->prev){
        if(curr_meta->prev->is_free){   //if there is a prev, and its free
            size_t size_free = curr_meta->total_size + curr_meta->prev->total_size - sizeof(MallocMetaData);
            if(size_free>size){ //check if there is enough free space with both blocks combined
                _removeNodeFromBin(curr_meta->prev);    //remove prev from the bin
                return _mergeBlock(curr_meta->prev, curr_meta); // merge
            }
            if(!curr_meta->next){
                _removeNodeFromBin(curr_meta->prev);
                _mergeBlock(curr_meta->prev, curr_meta);
                _wilderness(size);
            }
        }
    }
    if(curr_meta->next){    //else try to merge the next
        if(curr_meta->next->is_free){
            size_t size_free = curr_meta->total_size + curr_meta->next->total_size - sizeof(MallocMetaData);
            if(size_free>size){
                _removeNodeFromBin(curr_meta->next);
                return _mergeBlock(curr_meta, curr_meta->next);
            }
        }
    }
    if(curr_meta->next && curr_meta->prev){ //else try to merge both
        if(curr_meta->next->is_free && curr_meta->prev->is_free){
            size_t size_free = curr_meta->total_size + curr_meta->next->total_size + curr_meta->prev->total_size - sizeof(MallocMetaData);
            if(size_free>size){
                _removeNodeFromBin(curr_meta->next);
                _mergeBlock(curr_meta, curr_meta->next);
                _removeNodeFromBin(curr_meta->prev);
                return _mergeBlock(curr_meta->prev, curr_meta);
            }
        }
    }
    return NULL;
}

void* _wilderness(size_t size){ //used to expand the wilderness node
    size_t size_needed = size - last_aloc_node->total_size + sizeof(MallocMetaData);
    if(sbrk(size_needed) == (void*)(-1)){
        return  NULL;
    }
    if(last_aloc_node->is_free){    // smalloc and realloc
        _removeNodeFromBin(last_aloc_node);
    }
    num_allocated_bytes+=size_needed;
    last_aloc_node->total_size += size_needed;
    return (static_cast<unsigned char*>((void*)last_aloc_node) + sizeof(MallocMetaData));
}

void* _smalloc(size_t size){ // this is the case where we actually need to malloc a block
    size_t total_size = size + sizeof(MallocMetaData);
    if(last_aloc_node){ //if there is a last allocated node, and its free
        if(last_aloc_node->is_free){
            return _wilderness(size);
        }
    }
    MallocMetaData* new_node = (MallocMetaData*)sbrk(sizeof(MallocMetaData));   //else allocate
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

void* _mmap(size_t size){
    size_t total_size = size + sizeof(MallocMetaData);
    void* map = mmap(NULL, total_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if(map == (void*)(-1)){
        return NULL;
    }
    MallocMetaData* mmaped_node = (MallocMetaData*)(map);
    mmaped_node->total_size = total_size;
    mmaped_node->prev_bin_list = NULL;
    mmaped_node->next_bin_list = NULL;
    mmaped_node->prev = NULL;
    mmaped_node->next = NULL;
    mmaped_node->is_free = false;
    num_allocated_bytes+=size;
    num_allocated_blocks++;
    return static_cast<unsigned char*>(map) + sizeof(MallocMetaData);
}

MallocMetaData* _checkAndUpdateBin(size_t size){    //used to see if there is an available free block of appropriate size
    size_t total_size = size + sizeof(MallocMetaData);
    //check if there is a free block of appropiate size in the list at out initial index
    size_t init_index = size/1024;
    MallocMetaData* curr_node = NULL;
    if(bin[init_index]){    //if our list in the histogram isnt empty see if there is a big enough block
        curr_node = bin[init_index];
        while (curr_node != NULL) {
            if (curr_node->total_size >= total_size){   //if this block is big enough
                break;
            }
            curr_node = curr_node->next;
        }
    }
    if(!curr_node){ // if we didnt find a large enough block in the list, go over the rest of the bin
        for(size_t index = init_index+1; index<128; index++){
            if(bin[index]){
                curr_node = bin[index]; // after the first list the block is for sure large enough
                break;
            }
        }
    }
    if(curr_node){  // if we found a node that is large enough
        _removeNodeFromBin(curr_node);  //remove it from the bin
        _splitBlock(curr_node, size);   //split it if necessary
        return curr_node;
    }
    return NULL;
}

void* smalloc(size_t size)
{
    if(!init){//initialize the freeblock array, if it wasnt initialized
        init = true;
        for (auto & i : bin){
            i = NULL;
        }
    }
    size_t total_size = size + sizeof(MallocMetaData);
    if (size == 0 || size >= 100000000)//check that size is valid
        return NULL;
    if(size>=128*1024)
        return _mmap(size);
    MallocMetaData* curr_node = _checkAndUpdateBin(size);
    if(!curr_node) // there are no free blocks of appropriate size in our bin
        return _smalloc(size);  //we didnt return this at the beginning important
    else //there was a free node of appropriate size
        return static_cast<unsigned char*>((void*)(curr_node)) + sizeof(MallocMetaData);
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
    if(curr_meta->total_size - sizeof(MallocMetaData) >= 128*1024){   //if this was mmaped
        num_allocated_blocks--;
        num_allocated_bytes-=(curr_meta->total_size- sizeof(MallocMetaData));
        munmap(curr_meta, curr_meta->total_size);
        return;
    }
    if(curr_meta->is_free)
        return;
    MallocMetaData *next = curr_meta->next;
    if(next){   // there is a next, and its free we need to merge it
        if(next->is_free){
            _removeNodeFromBin(next);
            _mergeBlock(curr_meta, curr_meta->next);
            }
    }
    MallocMetaData *prev = curr_meta->prev;
    if(prev){
        if(prev->is_free){  //same thing, merge the blocks
            _removeNodeFromBin(prev);
            _mergeBlock(prev, curr_meta);
            curr_meta = prev;
        }
    }
    _insertNodeToBin(curr_meta);
}

void* srealloc(void* oldp, size_t size)
{
    if (size == 0 || size >= 100000000)
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
    if(old_size == size)
        return oldp;
    if(size >= 128*1024){   //if we want more size than 128kb, mmap it
        void* newmap = _mmap(size);
        if(!newmap)
            return NULL;
        if (old_size > size)
            memcpy(newmap, oldp, size);
        else
            memcpy(newmap, oldp, old_size); //mmap returns a pointer to the data, so copy there
        sfree(oldp);
        return newmap;
    }
    if(total_size < curr_meta->total_size) //if we have enough size, than use it
    {
        _splitBlock(curr_meta, size);
        return oldp;
    }

    MallocMetaData* merged_block = _superMerge(curr_meta, size);  //try to merge it with the adjacent blocks
    if(merged_block){
        _splitBlock(merged_block, size);
        return static_cast<unsigned char*>((void*)(merged_block)) + sizeof(MallocMetaData); //we returned the metadata here
    }
    if(curr_meta == last_aloc_node){
        return _wilderness(size);
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

//int main()
//{
//    void* array[23];
//    for (int i = 0 ; i < 23 ; ++i) {
//        (array[i] = smalloc((128*1024 + 1)));
//    }
//
//    for (int i = 0 ; i < (128*1024 + 1) ; ++i) {
//        ((char *) array[0])[i] = 'b';
//        ((char *) array[1])[i] = 'a';
//    }
//    sfree(array[0]);
//
//    (array[1] = srealloc(array[1], (128*1024 + 1) * 2));
//
//    for (int i = 0 ; i < (128*1024 + 1) ; ++i) {
//        if (((char *) array[1])[i] != 'a') {
//            std::cout << "realloc didnt copy the char a to index " << i << std::endl;
//            break;
//        }
//    }
//
//    (array[1] = srealloc(array[1], (128*1024 + 1))); //test  decreasing
//
//    for (int i = 0 ; i < (128*1024 + 1) ; ++i) {
//        if (((char *) array[1])[i] != 'a') {
//            std::cout << "realloc didnt copy the char a to index " << i << std::endl;
//            break;
//        }
//    }
//}