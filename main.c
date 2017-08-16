#include "cache.h"

int main()
{		
	//cache_run_lru("hm_1.ascii","./result/out.lru","./result/smr.lru","./result/ssd.lru");
	//cache_run_larc("hm_1.ascii","./result/out.larc","./result/smr.larc","./result/ssd.larc");
	cache_run_zac("hm_1.ascii","./result/out.zac","./result/smr.zac","./result/ssd.zac");
	return 1;
}

