//
// Created by student on 1/23/22.
//

#ifndef OSWET4_MALLOC_2_H
#define OSWET4_MALLOC_2_H
void* smalloc(size_t size);
void* scalloc(size_t num, size_t size);
void sfree(void* p);
void* srealloc(void* oldp, size_t size);
size_t _num_free_blocks();
size_t _num_free_bytes();
size_t _num_allocated_blocks();
size_t _num_allocated_bytes();
size_t _num_meta_data_bytes();
size_t _size_meta_data();
#endif //OSWET4_MALLOC_2_H
