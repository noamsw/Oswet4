#include <unistd.h>
void* smalloc(size_t size)
{
    if(!size || size > 100000000
    {
        return NULL;
    }
    void *p = sbrk(size);
    if(p == (void*)(-1)){
        return NULL;
    }
    return p;
}
