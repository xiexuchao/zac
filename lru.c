#include "cache.h"


void cache_init_lru(struct cache_info *cache,char *trace, char *output, char *smrTrc, char *ssdTrc)
{
	cache->size_block=4;	//KB
	cache->size_cache=1024;	//MB
	
	cache->blk_trc_all=0;
	
	//cache->blk_max_all=1024*cache->size_cache/cache->size_block;
	cache->blk_max_all=30;
	cache->blk_max_reg=0;
	cache->blk_max_evt=0;
	cache->blk_max_gst=0;
	
	cache->blk_now_all=0;
	cache->blk_now_reg=0;
	cache->blk_now_evt=0;
	cache->blk_now_gst=0;
	
	cache->set_now_evt=0;
	
	cache->hit_all_all=0;
	cache->hit_red_all=0;
	cache->hit_wrt_all=0;
	cache->hit_all_reg=0;
	cache->hit_red_reg=0;
	cache->hit_wrt_reg=0;
	cache->hit_all_evt=0;
	cache->hit_red_evt=0;
	cache->hit_wrt_evt=0;
	cache->hit_all_gst=0;
	cache->hit_red_gst=0;
	cache->hit_wrt_gst=0;
	
	
	cache->req=(struct req_info *)malloc(sizeof(struct req_info));
	alloc_assert(cache->req,"cache->req");
	memset(cache->req,0,sizeof(struct req_info));

    cache->blk_head_reg=NULL;
    cache->blk_tail_reg=NULL;
    cache->blk_head_gst=NULL;
    cache->blk_tail_gst=NULL;
    cache->set_head_evt=NULL;
    cache->set_tail_evt=NULL;
        
    strcpy(cache->filename_trc,trace);
    strcpy(cache->filename_out,output);
    strcpy(cache->filename_smr,smrTrc);
    strcpy(cache->filename_ssd,ssdTrc);
    
    cache->file_trc=fopen(cache->filename_trc,"r");
    cache->file_out=fopen(cache->filename_out,"w");
    cache->file_smr=fopen(cache->filename_smr,"w");
    cache->file_ssd=fopen(cache->filename_ssd,"w");    
}

void cache_lru(struct cache_info *cache)
{
	cache->blk_trc_all += cache->req->size;
	
	if(cache->req->type == WRITE)
	{
		while(cache->req->size)
		{
			if(cache_blk_lru(cache,cache->req->blkn,WRITE) == SUCCESS)
			{
				cache->hit_all_all++;
				cache->hit_wrt_all++;
			}
			else
			{
				cache->blk_now_all++;
			}
			while(cache->blk_now_all > cache->blk_max_all)
			{
				cache_delete_tail_blk_reg(cache);
				cache->blk_now_all--;
			}
			cache->req->size--;
			cache->req->blkn++;
		}//while
	}
	else if(cache->req->type == READ)
	{
		while(cache->req->size)
		{
			if(cache_blk_lru(cache,cache->req->blkn,READ) == SUCCESS)
			{
				cache->hit_all_all++;
				cache->hit_red_all++;
			}
			else
			{
				cache->blk_now_all++;
			}
			while(cache->blk_now_all > cache->blk_max_all)
			{
				cache_delete_tail_blk_reg(cache);
				cache->blk_now_all--;
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
int cache_blk_lru(struct cache_info *cache,unsigned int blkn,unsigned int state)
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
	alloc_assert(block,"block");
	memset(block,0,sizeof(struct blk_info));
	
	block->blkn=blkn;
	block->state=state;
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


void cache_print_lru(struct cache_info *cache)
{
	struct blk_info *index;
	
	printf("------------------------\n");
	printf("Cache Max blk = %d\n",cache->blk_max_all);
	printf("Cache Now blk = %d\n",cache->blk_now_all);
	printf("Cache Trc blk = %d\n",cache->blk_trc_all);
	printf("Cache Hit all = %d\n",(cache->hit_red_all + cache->hit_wrt_all));
	printf("Cache Hit Red = %d\n",cache->hit_red_all);
	printf("Cache Hit Wrt = %d\n",cache->hit_wrt_all);
	printf("------------------------\n");
	fprintf(cache->file_out,"Cache Max blk = %d\n",cache->blk_max_all);
	fprintf(cache->file_out,"Cache Now blk = %d\n",cache->blk_now_all);
	fprintf(cache->file_out,"Cache Trc blk = %d\n",cache->blk_trc_all);
	fprintf(cache->file_out,"Cache Hit all = %d\n",(cache->hit_red_all + cache->hit_wrt_all));
	fprintf(cache->file_out,"Cache Hit Red = %d\n",cache->hit_red_all);
	fprintf(cache->file_out,"Cache Hit Wrt = %d\n",cache->hit_wrt_all);
	
	index=cache->blk_head_reg;
	while(index)
	{
		fprintf(cache->file_out,"%-5d \n",index->blkn);
		index=index->blk_next;
	}
}
