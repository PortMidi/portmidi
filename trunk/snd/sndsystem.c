/* sndsystem.c
 *
 * Roger Dannenberg
 * 21 Jun 1997
 *
 */

#include "stdlib.h"
#include "stdio.h"

#include "snd.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __WX__
void snd_fail(char *msg)
{
    printf("ERROR: %s\n", msg);
}
#endif

void *snd_alloc(size_t s)
{
    void *res;
    /* printf("snd_alloc of %ld ", s); */
    res = malloc(s);
    return res;
}


void snd_free(void *a)
{
    /* printf("snd_free of %lx\n", a);*/
    free(a);
}

#ifdef __cplusplus
} // extern "C"
#endif

