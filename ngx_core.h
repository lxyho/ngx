#ifndef _NGX_CORE_H_INCLUDE_
#define _NGX_CORE_H_INCLUDE_

/*
 * msvc and icc7 compile memset() to the inline "rep stos"
 * while ZeroMemory() and bzero() are the calls.
 * icc7 may also inline several mov's of a zeroed register for small blocks.
 */
#define ngx_memzero(buf, n)       (void) memset(buf, 0, n)
#define ngx_memset(buf, c, n)     (void) memset(buf, c, n)

typedef struct ngx_pool_s      ngx_pool_t;
typedef struct ngx_chain_s     ngx_chain_t;
typedef struct ngx_log_s       ngx_log_t;

#define  NGX_OK          0
#define  NGX_ERROR      -1
#define  NGX_AGAIN      -2
#define  NGX_BUSY       -3
#define  NGX_DONE       -4
#define  NGX_DECLINED   -5
#define  NGX_ABORT      -6


#include "ngx_alloc.h"


#endif