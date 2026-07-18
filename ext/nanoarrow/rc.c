#include <stdatomic.h>
#include <stdlib.h>

#include "ext.h"

struct rc {
    void* ptr;
    _Atomic int count;
    void (*destructor)(void*);
};

rc_t* rc_new(void* ptr, void (*destructor)(void*))
{
    rc_t* rc = malloc(sizeof(rc_t));
    rc->ptr = ptr;
    rc->count = 1;
    rc->destructor = destructor;
    return rc;
}

void rc_increment(rc_t* rc)
{
    rc->count++;
}

void rc_decrement(rc_t* rc)
{
    int count = --rc->count;
    if (count == 0)
    {
        rc->destructor(rc->ptr);
        free(rc);
    }
}
