//
// Created by student on 1/24/22.
//

#ifndef OSWET4_MALLOC_3_H
#define OSWET4_MALLOC_3_H
struct MallocMetaData;
void _insertNodeToBin(MallocMetaData* node);
void _removeNodeFromBin(MallocMetaData* node);
void _splitBlock(MallocMetaData* node, size_t size);
MallocMetaData* _mergeBlock(MallocMetaData* low_node, MallocMetaData* high_node);
MallocMetaData* _superMerge(MallocMetaData* curr_meta, size_t size);
void* _wilderness(size_t size);
void* _smalloc(size_t size);
void* _mmap(size_t size);
MallocMetaData* _checkAndUpdateBin(size_t size);
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
#endif //OSWET4_MALLOC_3_H
