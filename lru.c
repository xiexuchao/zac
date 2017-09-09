#include "cache.h"

void lru_init(struct cache_info *cache,char *trace, char *output, char *smrTrc, char *ssdTrc, unsigned int ssdsize)
{
	cache->size_block=4;		//KB
	cache->size_cache=ssdsize;	//MB
	
	cache->blk_trc_all=0;
	cache->blk_trc_red=0;
	cache->blk_trc_wrt=0;
	
	cache->blk_ssd_wrt=0;
	
	cache->blk_max_all=1024*cache->size_cache/cache->size_block;
	cache->blk_max_reg=cache->blk_max_all;
	
	cache->blk_now_reg=0;
	
	cache->hit_red_reg=0;
	cache->hit_wrt_reg=0;	
	
	cache->req=(struct req_info *)malloc(sizeof(struct req_info));
	cache_alloc_assert(cache->req,"cache->req");
	memset(cache->req,0,sizeof(struct req_info));

    cache->blk_head_reg=NULL;
    cache->blk_tail_reg=NULL;
        
    strcpy(cache->filename_trc,trace);
    strcpy(cache->filename_out,output);
    strcpy(cache->filename_smr,smrTrc);
    strcpy(cache->filename_ssd,ssdTrc);
    
    cache->file_trc=fopen(cache->filename_trc,"r");
    cache->file_out=fopen(cache->filename_out,"w");
    cache->file_smr=fopen(cache->filename_smr,"w");
    cache->file_ssd=fopen(cache->filename_ssd,"w");    
}

void lru_main(struct cache_info *cache)
{
	if(cache->req->type == WRITE)
	{
		while(cache->req->size)
		{
			if(lru_check_reg(cache,cache->req->blkn,WRITE) == SUCCESS)
			{
				cache->blk_ssd_wrt++;
				cache->hit_wrt_reg++;
				fprintf(cache->file_ssd,"%lld 1 %d\n",cache->req->blkn%cache->blk_max_all,WRITE);
			}
			else
			{
				cache->blk_ssd_wrt++;
				cache->blk_now_reg++;
				while(cache->blk_now_reg > cache->blk_max_reg)
				{
					lru_delete_tail_blk_reg(cache);
					cache->blk_now_reg--;
				}
				fprintf(cache->file_ssd,"%lld 1 %d\n",cache->req->blkn%cache->blk_max_all,WRITE);
			}
			cache->req->size--;
			cache->req->blkn++;
		}//while
	}
	else if(cache->req->type == READ)
	{
		while(cache->req->size)
		{
			if(lru_check_reg(cache,cache->req->blkn,READ) == SUCCESS)
			{
				cache->hit_red_reg++;
				fprintf(cache->file_ssd,"%lld 1 %d\n",cache->req->blkn%cache->blk_max_all,READ);
			}
			else
			{
				cache->blk_ssd_wrt++;
				cache->blk_now_reg++;
				while(cache->blk_now_reg > cache->blk_max_reg)
				{
					lru_delete_tail_blk_reg(cache);
					cache->blk_now_reg--;
				}
				fprintf(cache->file_smr,"%lld 1 %d\n",cache->req->blkn,READ);
				fprintf(cache->file_ssd,"%lld 1 %d\n",cache->req->blkn%cache->blk_max_all,WRITE);
			}
			cache->req->size--;
			cache->req->blkn++;
		}//while
	}//else
	else
	{
		printf("ERROR: Wrong Request Type! \n");
		exit(-1);
	}
}

/**
For Cache
	hit: search, delete, insert to head
	miss: build a new blk, insert to head
**/
int lru_check_reg(struct cache_info *cache,unsigned int blkn,unsigned int state)
{
	struct blk_info *index;
	struct blk_info *block;

	index = cache->blk_head_reg;
	while(index)
	{
		if(index->blkn == blkn)
		{	
			//delete and insert this blk to the head
			if(index == cache->blk_head_reg)
			{
				return SUCCESS;
			}
			else if(index == cache->blk_tail_reg)
			{	
				//delete from tail
				cache->blk_tail_reg = cache->blk_tail_reg->blk_prev;
				cache->blk_tail_reg->blk_next = NULL;
				//insert to head
				index->blk_prev = NULL;
				index->blk_next = cache->blk_head_reg;
				cache->blk_head_reg->blk_prev = index;
				cache->blk_head_reg = index;
			}
			else
			{
				//delete from list middle
				index->blk_prev->blk_next=index->blk_next;
				index->blk_next->blk_prev=index->blk_prev;
				//insert to head
				index->blk_prev = NULL;
				index->blk_next = cache->blk_head_reg;
				cache->blk_head_reg->blk_prev = index;
				cache->blk_head_reg = index;
			}
			return SUCCESS;
		}//if
		index = index->blk_next;
	}
	
	// build a new blk and add to head
	block=(struct blk_info *)malloc(sizeof(struct blk_info)); 
	cache_alloc_assert(block,"block");
	memset(block,0,sizeof(struct blk_info));
	
	block->blkn = blkn;
	block->setn = blkn/65536;
	block->state = state;
	
	if(cache->blk_head_reg == NULL)
	{
		block->blk_prev = NULL;
		block->blk_next = NULL;
		cache->blk_head_reg = block;
		cache->blk_tail_reg = block;
	}
	else
	{
		block->blk_prev = NULL;
		block->blk_next = cache->blk_head_reg;
		cache->blk_head_reg->blk_prev = block;
		cache->blk_head_reg = block;
	}
	return FAILURE;
}


void lru_delete_tail_blk_reg(struct cache_info *cache)
{
	struct blk_info *block;
	
	block = cache->blk_tail_reg;
	if(block != cache->blk_head_reg)
	{
		cache->blk_tail_reg = cache->blk_tail_reg->blk_prev;
		cache->blk_tail_reg->blk_next = NULL;
	}
	else
	{
		cache->blk_tail_reg = NULL;
		cache->blk_head_reg = NULL;
	}
	
	if(block->state == DIRTY)
	{
		fprintf(cache->file_smr,"%lld 1 %d\n",block->blkn,WRITE);
	}
	
	free(block);
}

void lru_print(struct cache_info *cache)
{
	printf("----------------------------------------------\n");
	printf("----------------------------------------------\n");
	printf("Cache Max blk Reg = %d\n",cache->blk_max_all);
	printf("Cache Now blk Reg = %d\n",cache->blk_now_reg);
	printf("Cache Trc all blk = %d\n",cache->blk_trc_all);
	printf("Cache Trc red blk = %d\n",cache->blk_trc_red);
	printf("Cache Trc wrt blk = %d\n",cache->blk_trc_wrt);
	printf("Write Traffic SSD = %d\n",cache->blk_ssd_wrt);
	printf("------\n");
	printf("Cache Hit All = %d || All Hit Ratio = %Lf \n",
			(cache->hit_red_reg + cache->hit_wrt_reg),
			(long double)(cache->hit_red_reg + cache->hit_wrt_reg)/(long double)cache->blk_trc_all);
	printf("Cache Hit Red = %d || Red Hit Ratio = %Lf\n",
			cache->hit_red_reg,(long double)cache->hit_red_reg/(long double)cache->blk_trc_red);
	printf("Cache Hit Wrt = %d || Wrt Hit Ratio = %Lf\n",
			cache->hit_wrt_reg,(long double)cache->hit_wrt_reg/(long double)cache->blk_trc_wrt);
	printf("------------------------\n");
}
