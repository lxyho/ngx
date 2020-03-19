#include <stdio.h>

#include "ngx_palloc.h"

int main() {
    printf("nginx memory pool");

	ngx_pool_t *pool = ngx_create_pool(1024, NULL);

    if (pool == NULL) 
	{
         printf("ngx create pool failed.");
         return 1;
    }

    return 0;
}