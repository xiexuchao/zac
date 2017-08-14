#include "cache.h"

int main()
{		
	//cache_run_lru("test.trc","./result/out.lru","./result/smr.lru","./result/ssd.lru");
	cache_run_larc("test.trc","./result/out.larc","./result/smr.larc","./result/ssd.larc");
	return 1;
}

