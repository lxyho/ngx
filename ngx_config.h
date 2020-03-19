#ifndef _NGX_CONFIG_H_INCLUDE_
#define _NGX_CONFIG_H_INCLUDE_


#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char   u_char;

typedef intptr_t        ngx_int_t;
typedef uintptr_t       ngx_uint_t;
typedef intptr_t        ngx_flag_t;

#ifndef ngx_inline
#define ngx_inline      inline
#endif

#ifndef NGX_ALIGNMENT
#define NGX_ALIGNMENT   sizeof(unsigned long)    /* platform word */
#endif

#define ngx_align_ptr(p, a)                                                   \
    (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))



#endif