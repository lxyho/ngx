#include "ngx_palloc.h"

static ngx_inline void *ngx_palloc_small(ngx_pool_t *pool, size_t size, ngx_uint_t align);
static void *ngx_palloc_block(ngx_pool_t *pool, size_t size);
static void *ngx_palloc_large(ngx_pool_t *pool, size_t size);

/* 创建一个内存池 */
ngx_pool_t *ngx_create_pool(size_t size, ngx_log_t *log)
{
	ngx_pool_t *p;
	p = ngx_memalign(NGX_POOL_ALIGNMENT, size, log);
	if (p == NULL) 
	{
		return NULL;
	}

	p->d.last = (u_char *)p + sizeof(ngx_pool_t);
	p->d.end = (u_char *)p + size;               
	p->d.next = NULL;                             
	p->d.failed = 0;                              

	size = size - sizeof(ngx_pool_t);
	p->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL;

	p->current = p;
	p->chain = NULL;
	p->large = NULL;
	p->cleanup = NULL;
	p->log = log;

	return p;
}

/* 销毁内存池 */
void ngx_destroy_pool(ngx_pool_t *pool) 
{
	ngx_pool_t          *p, *n;
	ngx_pool_large_t    *l;
	ngx_pool_cleanup_t  *c;

	// 首先清理pool->cleanup链表
	for (c = pool->cleanup; c; c = c->next) 
	{
		// handler 为一个清理的回调函数
		if (c->handler) 
		{
			//ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0, 
			//                "run cleanup: %p", c);
			c->handler(c->data);
		}
	}

#if (NGX_DEBUG)
	for (p = pool, n = pool->d.next; ; p = n, n = n->d.next) {
		ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, pool->log, 0, "free: %p, unused: %uz", p, p->d.next)
			if (n == NULL) {
				break;
			}
	}
#endif

	/* 清理pool->large 链表（pool->large为单独的大数据内存块）*/
	for (l = pool->large; l; l = l->next) 
	{
		if (l->alloc) 
		{
			ngx_free(l->alloc);
		}
	}

	/* 对内存池的data数据区域进行释放 */
	for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) 
	{
		ngx_free(p);
		if (n == NULL) 
		{
			break;
		}
	}
}

/* 重设内存池 */
void ngx_reset_pool(ngx_pool_t *pool) 
{
	ngx_pool_t       *p;
	ngx_pool_large_t *l;

	// 清理pool->large链表（pool->large为单独的大数据块内存块）
	for (l = pool->large; l; l = l->next) 
	{
		if (l->alloc) 
		{
			ngx_free(l->alloc);
		}
	}

	// 循环重新设置内存池data区域的 p->d.list；data区域数据并不清除
	for (p = pool; p; p = p->d.next) 
	{
		p->d.last = (u_char *)p + sizeof(ngx_pool_t);
		p->d.failed = 0;
	}

	pool->current = pool;
	pool->large = NULL;
	pool->large = NULL;
}

void *ngx_palloc(ngx_pool_t *pool, size_t size) 
{
#if !(NGX_DEBUG_PALLOC)
	if (size <= pool->max) 
	{
		return ngx_palloc_small(pool, size, 1);
	}
#endif
	return ngx_palloc_large(pool, size);
}

void *ngx_pnalloc(ngx_pool_t *pool, size_t size) 
{
#if !(NGX_DEBUG_PALLOC)
	if (size <= pool->max) 
	{
		return ngx_palloc_small(pool, size, 0);
	}
#endif
	return ngx_palloc_large(pool, size);
}

static ngx_inline void *ngx_palloc_small(ngx_pool_t *pool, size_t size, ngx_uint_t align) 
{
	u_char      *m;
	ngx_pool_t  *p;

	p = pool->current;

	do 
	{
		m = p->d.last;

		if (align) {
			m = ngx_align_ptr(m, NGX_ALIGNMENT);
		}

		if ((size_t)(p->d.end - m) >= size) {
			p->d.last = m + size;

			return m;
		}

		p = p->d.next;

	} while (p);
	return ngx_palloc_block(pool, size);
}

static void *ngx_palloc_block(ngx_pool_t *pool, size_t size) 
{
	u_char    *m;
	size_t     psize;
	ngx_pool_t *p, *new;

	psize = (size_t)(pool->d.end - (u_char *)pool);

	m = ngx_memalign(NGX_POOL_ALIGNMENT, psize, pool->log);
	if (m == NULL) 
	{
		return NULL;
	}

	new = (ngx_pool_t *)m;
	new->d.end = m + psize;
	new->d.next = NULL;
	new->d.failed = 0;

	m += sizeof(ngx_pool_data_t);
	m = ngx_align_ptr(m, NGX_ALIGNMENT);
	new->d.last = m + size;

	for (p = pool->current; p->d.next; p = p->d.next) 
	{
		if (p->d.failed++ > 4) 
		{
			pool->current = p->d.next;
		}
	}

	p->d.next = new;
	return m;
}

static void *ngx_palloc_large(ngx_pool_t *pool, size_t size)
{
	void              *p;
	ngx_uint_t         n;
	ngx_pool_large_t  *large;

	p = ngx_alloc(size, pool->log);
	if (p == NULL) 
	{
		return NULL;
	}

	n = 0;

	for (large = pool->large; large; large = large->next) 
	{
		if (large->alloc == NULL) 
		{
			large->alloc = p;
			return p;
		}

		if (n++ > 3) 
		{
			break;
		}
	}

	large = ngx_palloc_small(pool, sizeof(ngx_pool_large_t), 1);
	if (large == NULL) 
	{
		ngx_free(p);
		return NULL;
	}

	large->alloc = p;
	large->next = pool->large;
	pool->large = large;

	return p;
}


void *ngx_pmemalign(ngx_pool_t *pool, size_t size, size_t alignment)
{
	void              *p;
	ngx_pool_large_t  *large;

	p = ngx_memalign(alignment, size, pool->log);
	if (p == NULL) 
	{
		return NULL;
	}

	large = ngx_palloc_small(pool, sizeof(ngx_pool_large_t), 1);
	if (large == NULL) 
	{
		ngx_free(p);
		return NULL;
	}

	large->alloc = p;
	large->next = pool->large;
	pool->large = large;

	return p;
}


ngx_int_t ngx_pfree(ngx_pool_t *pool, void *p)
{
	ngx_pool_large_t  *l;

	for (l = pool->large; l; l = l->next) 
	{
		if (p == l->alloc) 
		{
			//ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0,
			//               "free: %p", l->alloc);
			ngx_free(l->alloc);
			l->alloc = NULL;

			return NGX_OK;
		}
	}

	return NGX_DECLINED;
}


void *ngx_pcalloc(ngx_pool_t *pool, size_t size)
{
	void *p;

	p = ngx_palloc(pool, size);
	if (p) 
	{
		ngx_memzero(p, size);
	}

	return p;
}


ngx_pool_cleanup_t *ngx_pool_cleanup_add(ngx_pool_t *p, size_t size)
{
	ngx_pool_cleanup_t  *c;

	c = ngx_palloc(p, sizeof(ngx_pool_cleanup_t));
	if (c == NULL) 
	{
		return NULL;
	}

	if (size) 
	{
		c->data = ngx_palloc(p, size);
		if (c->data == NULL) 
		{
			return NULL;
		}

	}
	else 
	{
		c->data = NULL;
	}

	c->handler = NULL;
	c->next = p->cleanup;

	p->cleanup = c;

	//ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, p->log, 0, "add cleanup: %p", c);

	return c;
}