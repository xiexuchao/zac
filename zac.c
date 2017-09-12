#include "cache.h"

void zac_init(struct cache_info *cache,char *trace, char *output, char *smrTrc, char *ssdTrc, unsigned int ssdsize)
{
	int i;
	
	cache->size_block=4;		//KB
	cache->size_cache=ssdsize;	//MB
	
	cache->blk_trc_all=0;
	cache->blk_trc_red=0;
	cache->blk_trc_wrt=0;
	
	cache->blk_ssd_wrt=0;
	cache->blk_inn_wrt=0;
		
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
	cache->set_num_evt=0;
	cache->set_blk_evt=0;
	
	cache->hit_red_reg=0;
	cache->hit_wrt_reg=0;
	cache->hit_red_evt=0;
	cache->hit_wrt_evt=0;
	cache->hit_red_gst=0;
	cache->hit_wrt_gst=0;
		
	cache->req=(struct req_info *)malloc(sizeof(struct req_info));
	cache_alloc_assert(cache->req,"cache->req");
	memset(cache->req,0,sizeof(struct req_info));

    cache->blk_head_reg=NULL;
    cache->blk_tail_reg=NULL;
    cache->blk_head_gst=NULL;
    cache->blk_tail_gst=NULL;
    cache->blk_head_evt=NULL;
    cache->blk_tail_evt=NULL;
        
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
		if(cache->set_size[block->setn] == 0)
		{
			cache->set_now_evt++;
		}
		cache->set_size[block->setn]++;
		cache->blk_now_evt++;	//check if a set need to be evicted
		while(cache->blk_now_evt > cache->blk_max_evt)
		{
			zac_delete_tail_set_evt(cache);
		}
				
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
	else
	{
		free(block);
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
				else// head == tail
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


void zac_delete_tail_set_evt(struct cache_info *cache)
{
	struct blk_info *index, *index2;
	unsigned int i=0,setn,max_size;
	
	setn = zac_find_max(cache);	
	max_size = cache->set_size[setn];
	//printf("+++++++The size of evicted set %d is %d ++++++++\n",setn, max_size);
	
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
				else// head == tail
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
			
			fprintf(cache->file_smr,"%lld 1 %d\n",index->blkn,WRITE);
			i++;
			cache->blk_now_evt--;
			cache->set_size[setn]--;
			index2 = index;
			index = index->blk_next;
			free(index2);			
		}//if
		else
		{
			index = index->blk_next;
		}
	}
	
	cache->set_num_evt++;
	cache->set_blk_evt+=max_size;
	
	if(i != max_size)
	{
		printf("+++++++++The blks evicted from EVT Cache %d != their set size %d !++++++++\n",i,max_size);
		printf("set %d size = %d \n",setn, cache->set_size[setn]);
		//cache->set_size[setn]=0;
		//exit(-1);
	}
	if(i > 0)
	{
		cache->set_now_evt--;
		if(cache->set_now_evt < 0)
		{
			printf("+++++++++A set is evicted from EVT Cache when EVT is empty !++++++++\n");
			//exit(-1);
		}
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


void zac_main(struct cache_info *cache)
{
	struct blk_info *block;
	
	if(cache->req->type == WRITE)
	{
		while(cache->req->size)
		{
			if(zac_check_reg(cache,cache->req->blkn,WRITE) == SUCCESS)
			{
				cache->blk_ssd_wrt++;
				cache->hit_wrt_reg++;
				fprintf(cache->file_ssd,"%lld 1 %d\n",cache->req->blkn%cache->blk_max_all,WRITE);
			}
			else if(zac_check_evt(cache,cache->req->blkn,WRITE) == SUCCESS) 
			{
				cache->blk_ssd_wrt++;
				//move from evicting cache to regular cache
				cache->hit_wrt_evt++;
				cache->blk_now_evt--;
				cache->set_size[cache->req->blkn/65536]--;
				if(cache->set_size[cache->req->blkn/65536] < 0)
				{
					printf("+++a set in EVT cache is hit when its size is 0 ?!+++++\n");
					exit(-1);
				}
				
				cache->blk_now_reg++;
				while(cache->blk_now_reg > cache->blk_max_reg)
				{
					zac_delete_tail_blk_reg(cache);
					cache->blk_now_reg--;
				}
				
				//evict the potential overlapped block in ghost cache
				if(zac_dedupe_blk_gst(cache,cache->req->blkn) == SUCCESS)
				{
					cache->blk_now_gst--;
				}
				fprintf(cache->file_ssd,"%lld 1 %d\n",cache->req->blkn%cache->blk_max_all,WRITE);
			}
			else
			{
				if(zac_check_gst(cache,cache->req->blkn,WRITE) == SUCCESS)
				{
					cache->blk_ssd_wrt++;
					//move this block from ghost cache to regular cache
					cache->hit_wrt_gst++;
					cache->blk_now_gst--;
					cache->blk_now_reg++;
					while(cache->blk_now_reg > cache->blk_max_reg)
					{
						zac_delete_tail_blk_reg(cache);
						cache->blk_now_reg--;
					}
					fprintf(cache->file_ssd,"%lld 1 %d\n",cache->req->blkn%cache->blk_max_all,WRITE);
				}
				else//insert to the head of ghost cache
				{
					//***********************************************************************************
					//can be eliminated by innocous NSW aware ghost caching******************************
					//***********************************************************************************
					cache->blk_ssd_wrt++;
					cache->blk_inn_wrt++;
					
					cache->blk_now_gst++;
					while(cache->blk_now_gst > cache->blk_max_gst)
					{
						zac_delete_tail_blk_gst(cache);
						cache->blk_now_gst--;
					}
					
					//***********************************************************************************
					//can be eliminated by innocous NSW aware ghost caching******************************
					//***********************************************************************************
					fprintf(cache->file_ssd,"%lld 1 %d\n",cache->req->blkn%cache->blk_max_all,WRITE);
					
					//cache its real data to evicting cache
					// build a new blk and add to the head of evt cache
					block=(struct blk_info *)malloc(sizeof(struct blk_info)); 
					cache_alloc_assert(block,"block");
					memset(block,0,sizeof(struct blk_info));
					
					block->blkn = cache->req->blkn;
					block->setn = cache->req->blkn/65536;
					block->state = DIRTY;
					
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
					
					if(cache->set_size[block->setn] == 0)
					{
						cache->set_now_evt++;
					}
					cache->set_size[block->setn]++;
					cache->blk_now_evt++;	//check if a set need to be evicted
					while(cache->blk_now_evt > cache->blk_max_evt)
					{
						zac_delete_tail_set_evt(cache);
						//cache->set_now_evt--;
					}
				}
			}//else
			cache->req->size--;
			cache->req->blkn++;
		}//while
	}//write
	else if(cache->req->type == READ)
	{
		while(cache->req->size)
		{
			if(zac_check_reg(cache,cache->req->blkn,READ) == SUCCESS)
			{
				cache->hit_red_reg++;
				fprintf(cache->file_ssd,"%lld 1 %d\n",cache->req->blkn%cache->blk_max_all,READ);
			}
			else if(zac_check_evt(cache,cache->req->blkn,READ) == SUCCESS) 
			{
				//move from evicting cache to regular cache
				cache->hit_red_evt++;
				cache->blk_now_evt--;
								
				cache->set_size[cache->req->blkn/65536]--;
				if(cache->set_size[cache->req->blkn/65536] < 0)
				{
					printf("+++a set in EVT cache is READ hit when its size is 0 ?!+++++\n");
					exit(-1);
				}
				
				cache->blk_now_reg++;
				while(cache->blk_now_reg > cache->blk_max_reg)
				{
					zac_delete_tail_blk_reg(cache);
					cache->blk_now_reg--;
				}
				
				//evict the potential overlapped block in ghost cache
				if(zac_dedupe_blk_gst(cache,cache->req->blkn) == SUCCESS)
				{
					cache->blk_now_gst--;
				}
				fprintf(cache->file_ssd,"%lld 1 %d\n",cache->req->blkn%cache->blk_max_all,READ);
			}
			else
			{
				if(zac_check_gst(cache,cache->req->blkn,READ) == SUCCESS)
				{
					cache->blk_ssd_wrt++;
					//move this block from ghost cache to regular cache
					cache->hit_red_gst++;
					cache->blk_now_gst--;
					cache->blk_now_reg++;
					while(cache->blk_now_reg > cache->blk_max_reg)
					{
						zac_delete_tail_blk_reg(cache);
						cache->blk_now_reg--;
					}					
					fprintf(cache->file_smr,"%lld 1 %d\n",cache->req->blkn,READ);
					fprintf(cache->file_ssd,"%lld 1 %d\n",cache->req->blkn%cache->blk_max_all,WRITE);
				}
				else//insert to the head of ghost cache
				{
					cache->blk_now_gst++;
					while(cache->blk_now_gst > cache->blk_max_gst)
					{
						zac_delete_tail_blk_gst(cache);
						cache->blk_now_gst--;
					}
					fprintf(cache->file_smr,"%lld 1 %d\n",cache->req->blkn,READ);
				}
			}//else
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
For Regular Cache
	hit: search, delete, insert to head
	miss: go to check evicting cache
**/
int zac_check_reg(struct cache_info *cache,unsigned int blkn,unsigned int state)
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
int zac_check_evt(struct cache_info *cache,unsigned int blkn,unsigned int state)
{
	struct blk_info *index;
	struct blk_info *block;
	unsigned int setn = blkn/65536;
	
	if(cache->set_size[setn] == 0)
	{
		return FAILURE;	
	}
	else
	{
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
			
				return SUCCESS;
			}//if
			index = index->blk_next;
		}
	}
	return FAILURE;
}

/**
For Ghost Cache
	hit: search, delete, insert to regular cache
	miss: build a new blk, insert to the head of ghost cache
**/
int zac_check_gst(struct cache_info *cache,unsigned int blkn,unsigned int state)
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
				else// head == tail
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
						
			return SUCCESS;
		}//if
		index = index->blk_next;
	}
	
	//insert to the head of ghost cache
	// build a new blk and add to head
	block=(struct blk_info *)malloc(sizeof(struct blk_info)); 
	cache_alloc_assert(block,"block");
	memset(block,0,sizeof(struct blk_info));
	
	block->blkn = blkn;
	block->setn = blkn/65536;
	block->state = state;
			
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



void zac_print(struct cache_info *cache)
{
	printf("----------------------------------------------\n");
	printf("----------------------------------------------\n");
	printf("Cache Max blk Reg = %d\n",cache->blk_max_reg);
	printf("Cache Max blk Evt = %d\n",cache->blk_max_evt);
	printf("Cache Max blk Gst = %d\n",cache->blk_max_gst);
	printf("Cache Now blk Reg = %d\n",cache->blk_now_reg);
	printf("Cache Now blk Evt = %d\n",cache->blk_now_evt);
	printf("Cache Now blk Gst = %d\n",cache->blk_now_gst);
	printf("Cache Now set Evt = %d\n",cache->set_now_evt);
	printf("Cache Trc all blk = %d\n",cache->blk_trc_all);
	printf("Cache Trc red blk = %d\n",cache->blk_trc_red);
	printf("Cache Trc wrt blk = %d\n",cache->blk_trc_wrt);
	printf("Write Traffic SSD = %d\n",cache->blk_ssd_wrt);
	printf("Write InnBlk  SSD = %d\n",cache->blk_inn_wrt);
	printf("------\n");
	printf("Cache Hit All = %d || All Hit Ratio = %Lf \n",
			(cache->hit_red_reg+cache->hit_wrt_reg+cache->hit_red_evt+cache->hit_wrt_evt),
			(long double)(cache->hit_red_reg+cache->hit_wrt_reg+cache->hit_red_evt+cache->hit_wrt_evt)/(long double)cache->blk_trc_all);
	printf("Cache Hit Red = %d || Red Hit Ratio = %Lf\n",
			(cache->hit_red_reg+cache->hit_red_evt),(long double)(cache->hit_red_reg+cache->hit_red_evt)/(long double)cache->blk_trc_red);
	printf("Cache Hit Wrt = %d || Wrt Hit Ratio = %Lf\n",
			(cache->hit_wrt_reg+cache->hit_wrt_evt),(long double)(cache->hit_wrt_reg+cache->hit_wrt_evt)/(long double)cache->blk_trc_wrt);
	printf("----\n");
	printf("Cache Hit all Reg = %d\n",(cache->hit_red_reg + cache->hit_wrt_reg));
	printf("Cache Hit Red Reg = %d\n",cache->hit_red_reg);
	printf("Cache Hit Wrt Reg = %d\n",cache->hit_wrt_reg);
	printf("----\n");
	printf("Cache Hit all Evt = %d\n",(cache->hit_red_evt + cache->hit_wrt_evt));
	printf("Cache Hit Red Evt = %d\n",cache->hit_red_evt);
	printf("Cache Hit Wrt Evt = %d\n",cache->hit_wrt_evt);
	printf("----\n");
	printf("Cache Hit all Gst = %d\n",(cache->hit_red_gst + cache->hit_wrt_gst));
	printf("Cache Hit Red Gst = %d\n",cache->hit_red_gst);
	printf("Cache Hit Wrt Gst = %d\n",cache->hit_wrt_gst);
	printf("----\n");
	printf("Cache Evicted Sets = %d\n",cache->set_num_evt);
	printf("Cache Evicted Blks = %d\n",cache->set_blk_evt);
	printf("Cache Avg Set Size = %Lf\n",(long double)cache->set_blk_evt/(long double)cache->set_num_evt);
	
	printf("------------------------\n");
}
