#include <unistd.h>
void* smalloc(size_t size)
{
    if(!size || size > 100000000
    {
        return NULL;
    }
    return sbrk(size);
}
