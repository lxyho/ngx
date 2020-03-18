#include "ngx_palloc.h"

/* 封装了malloc 函数，并且添加了日志*/

void* ngx_alloc(size_t size, ngx_log_t *log) {
    void *p;
    p = malloc(size);
    if (p == NULL) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno, "malloc(%uz) failed", size);
    }
    ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, log, 0, "malloc: %p:%uz", p, size);
    return p;
}

/* 
* 调用ngx_alloc方法，如果分配成功，则调用ngx_memzero方法，将内存块设置为0
* #define ngx_memzero(buf, n) (void) memset(buf, 0, n)
*/
void ngx_calloc(size_t size, ngx_log_t *log) {
    void *p;
    // 调用内存分配函数
    p = ngx_alloc(size, log);
    if (p) {
        // 将内存块全部设置为0
        ngx_memzero(p, size);
    }
    return p;
}

/* 创建一个内存池 */
ngx_pool_t *ngx_create_pool(size_t size, ngx_log_t *log) {
    ngx_pool_t *p;
    // 相当于分配了一块内存 ngx_alloc(size, log)
    p = ngx_memalign(NGX_POOL_ALIGNMENT, size, log);
    if (p == NULL) {
        return NULL;
    }
    /*
    * Nginx会分配一块大内存，其中内存头部存放ngx_pool_t本身内存池的数据结构
    * ngx_pool_data_t      p->d 存放内存池的数据部分（适合小于p->max的内存块存储）
    * p->large 存放大内存块列表
    * p->cleanup 存放可以被回调函数清理的内存块（该内存块不一定会在内存池上面分配）
    */
   p->d.last = (u_char*)p + sizeof(ngx_pool_t); // 内存开始地址，指向ngx_pool_t结构体之后数据取起始位置
   p->d.end = (u_char*)p + size; // 内存地址结束
   p->d.next = NULL; // 下一个ngx_pool_t 内存池地址
   p->d.failed = 0; // 失败次数
   size = size - sizeof(ngx_pool_t);
   p->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL;

   /* 只有缓存池的父节点，才会用到下面这些，子节点只挂载在p->d.next，并且只负责p->d的数据内容*/
   p->current = p;
   p->chain = NULL;
   P->cleanup = NULL;
   p->log = log;
   return p;
} 