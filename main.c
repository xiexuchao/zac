#include "cache.h"

int main()
{		
//	cache_run_lru("hm_1.ascii","./result/out.lru","./result/smr.lru","./result/ssd.lru",1024);
//	cache_run_larc("hm_1.ascii","./result/out.larc","./result/smr.larc","./result/ssd.larc",1024);
//	cache_run_most("hm_1.ascii","./result/out.most","./result/smr.most","./result/ssd.most",64);
	cache_run_zac("hm_1.ascii","./result/out.zac","./result/smr.zac","./result/ssd.zac",64);
	return 1;
}

