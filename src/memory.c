#include "memory.h"


void *
xmalloc(unsigned n)
{
        void *p;
        p = GC_MALLOC_ATOMIC(n);
        if (p != NULL)
                return p;
        fprintf(stderr, "insufficient memory\n");
        assert(p != NULL);
        exit(EXIT_FAILURE);
}

void *
xcopy(void* src, size_t n)
{
        void *dst = xmalloc(n);
        memcpy(dst, src, n);
        return(dst);
}
