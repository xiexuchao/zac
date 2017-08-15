#include "cache.h"


void cache_init_zac(struct cache_info *cache,char *trace, char *output, char *smrTrc, char *ssdTrc)
{
	int i;
	
	cache->size_block=4;	//KB
	cache->size_cache=128;	//MB
	
	cache->blk_trc_all=0;
	
	cache->blk_max_all=1024*cache->size_cache/cache->size_block;
	cache->blk_max_reg=cache->blk_max_all*0.8;
	cache->blk_max_evt=cache->blk_max_all*0.2;
	cache->blk_max_gst=cache->blk_max_all;
	
	cache->blk_now_reg=0;
	cache->blk_now_evt=0;
	cache->blk_now_gst=0;
	
	cache->set_now_evt=0;
	for(i=0;i<30000;i++)
	{
		cache->set_size[i]=0;
	}
	
	cache->hit_red_reg=0;
	cache->hit_wrt_reg=0;
	cache->hit_red_evt=0;
	cache->hit_wrt_evt=0;
	cache->hit_red_gst=0;
	cache->hit_wrt_gst=0;
		
	cache->req=(struct req_info *)malloc(sizeof(struct req_info));
	alloc_assert(cache->req,"cache->req");
	memset(cache->req,0,sizeof(struct req_info));

    cache->blk_head_reg=NULL;
    cache->blk_tail_reg=NULL;
    cache->blk_head_gst=NULL;
    cache->blk_tail_gst=NULL;
        
    strcpy(cache->filename_trc,trace);
    strcpy(cache->filename_out,output);
    strcpy(cache->filename_smr,smrTrc);
    strcpy(cache->filename_ssd,ssdTrc);
    
    cache->file_trc=fopen(cache->filename_trc,"r");
    cache->file_out=fopen(cache->filename_out,"w");
    cache->file_smr=fopen(cache->filename_smr,"w");
    cache->file_ssd=fopen(cache->filename_ssd,"w");    
}


void zac_delete_tail_blk_reg(struct cache_info *cache)
{
	struct blk_info *block;
	unsigned int setn;
	
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
		//A dirty blocks is evicted and admitted to evicting cache
		setn = block->blkn/65536;
		if(cache->set_size[setn] == 0)
		{
			cache->set_now_evt++;
		}
		cache->set_size[setn]++;
		
		if(cache->blk_head_evt == NULL)
		{
			block->blk_prev = NULL;
			block->blk_next = NULL;
			cache->blk_head_evt = block;
			cache->blk_tail_evt = block;
		}
		else
		{
			block->blk_prev = NULL;
			block->blk_next = cache->blk_head_evt;
			cache->blk_head_evt->blk_prev = block;
			cache->blk_head_evt = block;
		}
		
	}
}

void zac_delete_tail_blk_gst(struct cache_info *cache)
{
	struct blk_info *block;
	
	block = cache->blk_tail_gst;
	if(block != cache->blk_head_gst)
	{
		cache->blk_tail_gst = cache->blk_tail_gst->blk_prev;
		cache->blk_tail_gst->blk_next = NULL;
	}
	else
	{
		cache->blk_tail_gst = NULL;
		cache->blk_head_gst = NULL;
	}
	
	free(block);
}


int zac_dedupe_blk_gst(struct cache_info *cache,unsigned int blkn)
{
	struct blk_info *index;
	
	index = cache->blk_head_gst;
	while(index)
	{
		if(index->blkn == blkn)
		{	
			//delete 
			if(index == cache->blk_head_gst)
			{
				//delete from head
				if(index->blk_next != NULL) //current ghost cache size > 1
				{
					cache->blk_head_gst = cache->blk_head_gst->blk_next;
					cache->blk_head_gst->blk_prev = NULL;
				}
				else	// head == tail
				{
					cache->blk_head_gst = NULL;
					cache->blk_tail_gst = NULL;
				}
			}
			else if(index == cache->blk_tail_gst)
			{	
				//delete from tail
				cache->blk_tail_gst = cache->blk_tail_gst->blk_prev;
				cache->blk_tail_gst->blk_next = NULL;
			}
			else
			{
				//delete from list middle
				index->blk_prev->blk_next=index->blk_next;
				index->blk_next->blk_prev=index->blk_prev;
			}
			free(index);
			return SUCCESS;
		}//if
		index = index->blk_next;
	}
	return FAILURE;
}


void zac_delete_tail_set_evt(struct cache_info *cache,unsigned int setn)
{
	struct blk_info *index;
	unsigned int i,max_size;
	
	max_size=cache->set_size[setn];
	
	index = cache->blk_head_evt;
	while(index)
	{
		if(index->setn == setn)
		{	
			//delete 
			if(index == cache->blk_head_evt)
			{
				//delete from head
				if(index->blk_next != NULL) //current ghost cache size > 1
				{
					cache->blk_head_evt = cache->blk_head_evt->blk_next;
					cache->blk_head_evt->blk_prev = NULL;
				}
				else	// head == tail
				{
					cache->blk_head_evt = NULL;
					cache->blk_tail_evt = NULL;
				}
			}
			else if(index == cache->blk_tail_evt)
			{	
				//delete from tail
				cache->blk_tail_evt = cache->blk_tail_evt->blk_prev;
				cache->blk_tail_evt->blk_next = NULL;
			}
			else
			{
				//delete from list middle
				index->blk_prev->blk_next=index->blk_next;
				index->blk_next->blk_prev=index->blk_prev;
			}
			free(index);
			i++;
		}//if
		index = index->blk_next;
	}
	if(i != max_size)
	{
		printf("The blks evicted from EVT Cache != their set size !\n");
		exit(-1);
	}
}

int zac_find_max(struct cache_info* cache)
{
	unsigned int i,max_index=0,max_size=0;
	for(i=0;i<30000;i++)
	{
		if(cache->set_size[i] > max_size)
		{
			max_size = cache->set_size[i];
			max_index = i;
		}
	}
	if(max_size == 0)
	{
		printf("Evicting a set with 0 size? \n");
		exit(-1);
	}
	return max_index;
}


void cache_zac(struct cache_info *cache)
{
	struct blk_info *block;
	
	cache->blk_trc_all += cache->req->size;
	
	if(cache->req->type == WRITE)
	{
		while(cache->req->size)
		{
			if(cache_blk_zac_reg(cache,cache->req->blkn,WRITE) == SUCCESS)
			{
				cache->hit_wrt_reg++;
			}
			else if(cache_blk_zac_evt(cache,cache->req->blkn,WRITE) == SUCCESS) 
			{
				//move from evicting cache to regular cache
				cache->hit_wrt_evt++;
				
				if(cache->blk_now_evt > 0)
				{
					cache->blk_now_evt--;
				}
				cache->blk_now_reg++;
				while(cache->blk_now_reg > cache->blk_max_reg)
				{
					cache_delete_tail_blk_reg(cache);
					cache->blk_now_reg--;
				}
				//evict the potential overlapped block in ghost cache
				zac_dedupe_blk_gst(cache,cache->req->blkn);
				
			}
			else
			{
				if(cache_blk_zac_gst(cache,cache->req->blkn,WRITE) == SUCCESS)
				{
					//move this block from ghost cache to regular cache
					cache->hit_wrt_gst++;
					
					if(cache->blk_now_gst > 0)
					{
						cache->blk_now_gst--;
					}
					cache->blk_now_reg++;
					while(cache->blk_now_reg > cache->blk_max_reg)
					{
						cache_delete_tail_blk_reg(cache);
						cache->blk_now_reg--;
					}
				}
				else//insert to the head of ghost cache
				{
					cache->blk_now_gst++;
					while(cache->blk_now_gst > cache->blk_max_gst)
					{
						cache_delete_tail_blk_gst(cache);
						cache->blk_now_gst--;
					}
					
					//cache its real data to evicting cache
					// build a new blk and add to the head of evt cache
					block=(struct blk_info *)malloc(sizeof(struct blk_info)); 
					alloc_assert(block,"block");
					memset(block,0,sizeof(struct blk_info));
					
					if(cache->blk_head_evt == NULL)
					{
						block->blk_prev = NULL;
						block->blk_next = NULL;
						cache->blk_head_evt = block;
						cache->blk_tail_evt = block;
					}
					else
					{
						block->blk_prev = NULL;
						block->blk_next = cache->blk_head_evt;
						cache->blk_head_evt->blk_prev = block;
						cache->blk_head_evt = block;
					}
				}
			}//else
			cache->req->size--;
			cache->req->blkn++;
		}//while
	}
	else if(cache->req->type == READ)
	{
		
	}//else
	else
	{
		printf("ERROR: Wrong Request Type! \n");
		exit(-1);
	}
}

/**
For Regular Cache
	hit: search, delete, insert to head
	miss: go to check evicting cache
**/
int cache_blk_zac_reg(struct cache_info *cache,unsigned int blkn,unsigned int state)
{
	struct blk_info *index;
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
	return FAILURE;
}


/**
For Evicting Cache
	hit: search, delete, insert to regular cache's head
	miss: go to check ghost cache
**/
int cache_blk_zac_evt(struct cache_info *cache,unsigned int blkn,unsigned int state)
{
	struct blk_info *index;
	struct blk_info *block;
	
	index = cache->blk_head_evt;
	while(index)
	{
		if(index->blkn == blkn)
		{	
			//delete and insert this blk to the head of regular cache
			if(index == cache->blk_head_evt)
			{
				//delete from head
				if(index->blk_next != NULL) //current evicting cache size > 1
				{
					cache->blk_head_evt = cache->blk_head_evt->blk_next;
					cache->blk_head_evt->blk_prev = NULL;
				}
				else	// head == tail
				{
					cache->blk_head_evt = NULL;
					cache->blk_tail_evt = NULL;
				}
			}
			else if(index == cache->blk_tail_evt)
			{	
				//delete from tail
				cache->blk_tail_evt = cache->blk_tail_evt->blk_prev;
				cache->blk_tail_evt->blk_next = NULL;
			}
			else
			{
				//delete from list middle
				index->blk_prev->blk_next=index->blk_next;
				index->blk_next->blk_prev=index->blk_prev;
			}
			free(index);
			
			// build a new blk and add to the head of regular cache
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
			
			//update the size of the corresponding set.
			
			
			return SUCCESS;
		}//if
		index = index->blk_next;
	}
	return FAILURE;
}

/**
For Ghost Cache
	hit: search, delete, insert to regular cache
	miss: build a new blk, insert to the head of ghost cache
**/
int cache_blk_zac_gst(struct cache_info *cache,unsigned int blkn,unsigned int state)
{
	struct blk_info *index;
	struct blk_info *block;

	index = cache->blk_head_gst;
	while(index)
	{
		if(index->blkn == blkn)
		{	
			//delete 
			if(index == cache->blk_head_gst)
			{
				//delete from head
				if(index->blk_next != NULL) //current ghost cache size > 1
				{
					cache->blk_head_gst = cache->blk_head_gst->blk_next;
					cache->blk_head_gst->blk_prev = NULL;
				}
				else	// head == tail
				{
					cache->blk_head_gst = NULL;
					cache->blk_tail_gst = NULL;
				}
			}
			else if(index == cache->blk_tail_gst)
			{	
				//delete from tail
				cache->blk_tail_gst = cache->blk_tail_gst->blk_prev;
				cache->blk_tail_gst->blk_next = NULL;
			}
			else
			{
				//delete from list middle
				index->blk_prev->blk_next=index->blk_next;
				index->blk_next->blk_prev=index->blk_prev;
			}
			free(index);
			
			// build a new blk and add to the head of regular cache
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
						
			return SUCCESS;
		}//if
		index = index->blk_next;
	}
	
	//insert to the head of ghost cache
	// build a new blk and add to head
	block=(struct blk_info *)malloc(sizeof(struct blk_info)); 
	alloc_assert(block,"block");
	memset(block,0,sizeof(struct blk_info));
	
	block->blkn=blkn;
	block->state=state;
	if(cache->blk_head_gst == NULL)
	{
		block->blk_prev = NULL;
		block->blk_next = NULL;
		cache->blk_head_gst = block;
		cache->blk_tail_gst = block;
	}
	else
	{
		block->blk_prev = NULL;
		block->blk_next = cache->blk_head_gst;
		cache->blk_head_gst->blk_prev = block;
		cache->blk_head_gst = block;
	}
	return FAILURE;
}



void cache_print_zac(struct cache_info *cache)
{
	struct blk_info *index;
	
	printf("------------------------\n");
	printf("Cache Max blk Reg = %d\n",cache->blk_max_reg);
	printf("Cache Max blk Gst = %d\n",cache->blk_max_gst);
	printf("Cache Now blk Reg = %d\n",cache->blk_now_reg);
	printf("Cache Now blk Gst = %d\n",cache->blk_now_gst);
	printf("Cache Trc blk = %d\n",cache->blk_trc_all);
	printf("Cache Hit all = %d\n",(cache->hit_red_reg + cache->hit_wrt_reg));
	printf("Cache Hit Red = %d\n",cache->hit_red_reg);
	printf("Cache Hit Wrt = %d\n",cache->hit_wrt_reg);
	printf("Cache Hit all Reg = %d\n",(cache->hit_red_reg + cache->hit_wrt_reg));
	printf("Cache Hit Red Reg = %d\n",cache->hit_red_reg);
	printf("Cache Hit Wrt Reg = %d\n",cache->hit_wrt_reg);
	printf("Cache Hit all Gst = %d\n",(cache->hit_red_gst + cache->hit_wrt_gst));
	printf("Cache Hit Red Gst = %d\n",cache->hit_red_gst);
	printf("Cache Hit Wrt Gst = %d\n",cache->hit_wrt_gst);
	
	printf("------------------------\n");
	
	fprintf(cache->file_out,"-----Regular Cache----- \n");
	index=cache->blk_head_reg;
	while(index)
	{
		fprintf(cache->file_out,"%-5d \n",index->blkn);
		index=index->blk_next;
	}
	fprintf(cache->file_out,"-----Ghost Cache----- \n");
	index=cache->blk_head_gst;
	while(index)
	{
		fprintf(cache->file_out,"%-5d \n",index->blkn);
		index=index->blk_next;
	}
}
