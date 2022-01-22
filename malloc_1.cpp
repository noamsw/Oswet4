#include <unistd.h>

void* smalloc(size_t size)
{
    if (size == 0 || size >= 100000000)
    {
        return NULL;
    }
    void* ret_val = sbrk(size);
    if (ret_val == (void*)(-1))
    {
        return NULL;
    }
    return ret_val;
}
