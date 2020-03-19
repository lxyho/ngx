#include <stdio.h>

#include "ngx_palloc.h"

int main() {
    printf("nginx memory pool");

	//ngx_os_init();

	ngx_pool_t *pool = ngx_create_pool(1024, NULL);

    if (pool == NULL) 
	{
         printf("ngx create pool failed.");
         return 1;
    }

	auto start = ngx_palloc(pool, 1025);
	auto start1 = ngx_palloc(pool, 10025);

	ngx_pfree(pool, start);

	ngx_destroy_pool(pool);


    return 0;
}