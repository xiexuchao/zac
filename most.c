#include "cache.h"

void most_init(struct cache_info *cache,char *trace, char *output, char *smrTrc, char *ssdTrc, unsigned int ssdsize)
{
	int i;
	
	cache->size_block=4;		//KB
	cache->size_cache=ssdsize;	//MB
	
	cache->blk_trc_all=0;
	cache->blk_trc_red=0;
	cache->blk_trc_wrt=0;
	
	cache->blk_ssd_wrt=0;
	
	cache->blk_max_all=1024*cache->size_cache/cache->size_block;
	cache->blk_max_evt=cache->blk_max_all;
	
	cache->blk_now_evt=0;
	cache->set_now_evt=0;
	
	for(i=0;i<30000;i++)
	{
		cache->set_size[i]=0;
	}
	cache->set_num_evt=0;
	cache->set_blk_evt=0;
	
	cache->hit_red_evt=0;
	cache->hit_wrt_evt=0;
		
	cache->req=(struct req_info *)malloc(sizeof(struct req_info));
	cache_alloc_assert(cache->req,"cache->req");
	memset(cache->req,0,sizeof(struct req_info));

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

void most_delete_max_set_evt(struct cache_info *cache)
{
	struct blk_info *index, *index2;
	unsigned int i=0,setn,max_size;
	
	setn = most_find_max(cache);	
	max_size = cache->set_size[setn];
	//printf("+++++++The size of evicted set is %d ++++++++\n",max_size);
	
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
			//free(index);
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
		exit(-1);
	}
	if(i > 0)
	{
		cache->set_now_evt--;
		if(cache->set_now_evt < 0)
		{
			printf("+++++++++A set is evicted from EVT Cache when EVT is empty !++++++++\n");
			exit(-1);
		}
	}
}

int most_find_max(struct cache_info* cache)
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


void most_main(struct cache_info *cache)
{
	struct blk_info *block;
	
	if(cache->req->type == WRITE)
	{
		while(cache->req->size)
		{
			if(most_check_evt(cache,cache->req->blkn,WRITE) == SUCCESS)
			{
				cache->blk_ssd_wrt++;
				cache->hit_wrt_evt++;
				fprintf(cache->file_ssd,"%lld 1 %d\n",cache->req->blkn%cache->blk_max_all,WRITE);
			}
			else//insert to the head of evicting cache
			{
				// build a new blk and add to the head of evicting cache
				block=(struct blk_info *)malloc(sizeof(struct blk_info)); 
				cache_alloc_assert(block,"block");
				memset(block,0,sizeof(struct blk_info));
	
				block->blkn = cache->req->blkn;
				block->setn = cache->req->blkn/65536;
				block->state = cache->req->type;
				
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
			
						
				cache->blk_ssd_wrt++;
				cache->blk_now_evt++;
				while(cache->blk_now_evt > cache->blk_max_evt)
				{
					most_delete_max_set_evt(cache);
					cache->blk_now_evt--;
				}
				fprintf(cache->file_ssd,"%lld 1 %d\n",cache->req->blkn%cache->blk_max_all,WRITE);
			}
			cache->req->size--;
			cache->req->blkn++;
		}//while
	}//write
	else if(cache->req->type == READ)
	{
		while(cache->req->size)
		{
			if(most_check_evt(cache,cache->req->blkn,READ) == SUCCESS)
			{
				cache->hit_red_evt++;
				fprintf(cache->file_ssd,"%lld 1 %d\n",cache->req->blkn%cache->blk_max_all,READ);
			}
			else //most will not proactively cache reads into evtular cache
			{
				fprintf(cache->file_smr,"%lld 1 %d\n",cache->req->blkn,READ);				
			}
			cache->req->size--;
			cache->req->blkn++;
		}//while
	}//read
	else
	{
		printf("ERROR: Wrong Request Type! \n");
		exit(-1);
	}
}

/**
For evicting Cache
	hit: search
	miss: go to read/write smr zone, zac will not proactively cache reads
**/
int most_check_evt(struct cache_info *cache,unsigned int blkn,unsigned int state)
{
	struct blk_info *index;
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
				return SUCCESS;
			}//if
			index = index->blk_next;
		}
	}
	
	return FAILURE;
}

void most_print(struct cache_info *cache)
{
	printf("----------------------------------------------\n");
	printf("----------------------------------------------\n");
	printf("Cache Max blk Evt = %d\n",cache->blk_max_evt);
	printf("Cache Now blk Evt = %d\n",cache->blk_now_evt);
	printf("Cache Now set Evt = %d\n",cache->set_now_evt);
	printf("Cache Trc all blk = %d\n",cache->blk_trc_all);
	printf("Cache Trc red blk = %d\n",cache->blk_trc_red);
	printf("Cache Trc wrt blk = %d\n",cache->blk_trc_wrt);
	printf("Write Traffic SSD = %d\n",cache->blk_ssd_wrt);
	printf("------\n");
	printf("Cache Hit All = %d || All Hit Ratio = %Lf \n",
			(cache->hit_red_evt+cache->hit_wrt_evt),
			(long double)(cache->hit_red_evt+cache->hit_wrt_evt)/(long double)cache->blk_trc_all);
	printf("Cache Hit Red = %d || Red Hit Ratio = %Lf\n",
			(cache->hit_red_evt),(long double)(cache->hit_red_evt)/(long double)cache->blk_trc_red);
	printf("Cache Hit Wrt = %d || Wrt Hit Ratio = %Lf\n",
			(cache->hit_wrt_evt),(long double)(cache->hit_wrt_evt)/(long double)cache->blk_trc_wrt);
	printf("----\n");
	printf("Cache Hit all Evt = %d\n",(cache->hit_red_evt + cache->hit_wrt_evt));
	printf("Cache Hit Red Evt = %d\n",cache->hit_red_evt);
	printf("Cache Hit Wrt Evt = %d\n",cache->hit_wrt_evt);
	printf("----\n");
	printf("Cache Evicted Sets = %d\n",cache->set_num_evt);
	printf("Cache Evicted Blks = %d\n",cache->set_blk_evt);
	printf("Cache Avg Set Size = %Lf\n",(long double)cache->set_blk_evt/(long double)cache->set_num_evt);
	printf("------------------------\n");
	
	fprintf(cache->file_out,"----------------------------------------------\n");
	fprintf(cache->file_out,"----------------------------------------------\n");
	fprintf(cache->file_out,"Cache Max blk Evt = %d\n",cache->blk_max_evt);
	fprintf(cache->file_out,"Cache Now blk Evt = %d\n",cache->blk_now_evt);
	fprintf(cache->file_out,"Cache Now set Evt = %d\n",cache->set_now_evt);
	fprintf(cache->file_out,"Cache Trc all blk = %d\n",cache->blk_trc_all);
	fprintf(cache->file_out,"Cache Trc red blk = %d\n",cache->blk_trc_red);
	fprintf(cache->file_out,"Cache Trc wrt blk = %d\n",cache->blk_trc_wrt);
	fprintf(cache->file_out,"Write Traffic SSD = %d\n",cache->blk_ssd_wrt);
	fprintf(cache->file_out,"------\n");
	fprintf(cache->file_out,"Cache Hit All = %d || All Hit Ratio = %Lf \n",
			(cache->hit_red_evt+cache->hit_wrt_evt),
			(long double)(cache->hit_red_evt+cache->hit_wrt_evt)/(long double)cache->blk_trc_all);
	fprintf(cache->file_out,"Cache Hit Red = %d || Red Hit Ratio = %Lf\n",
			(cache->hit_red_evt),(long double)(cache->hit_red_evt)/(long double)cache->blk_trc_red);
	fprintf(cache->file_out,"Cache Hit Wrt = %d || Wrt Hit Ratio = %Lf\n",
			(cache->hit_wrt_evt),(long double)(cache->hit_wrt_evt)/(long double)cache->blk_trc_wrt);
	fprintf(cache->file_out,"----\n");
	fprintf(cache->file_out,"Cache Hit all Evt = %d\n",(cache->hit_red_evt + cache->hit_wrt_evt));
	fprintf(cache->file_out,"Cache Hit Red Evt = %d\n",cache->hit_red_evt);
	fprintf(cache->file_out,"Cache Hit Wrt Evt = %d\n",cache->hit_wrt_evt);
	fprintf(cache->file_out,"----\n");
	fprintf(cache->file_out,"Cache Evicted Sets = %d\n",cache->set_num_evt);
	fprintf(cache->file_out,"Cache Evicted Blks = %d\n",cache->set_blk_evt);
	fprintf(cache->file_out,"Cache Avg Set Size = %Lf\n",(long double)cache->set_blk_evt/(long double)cache->set_num_evt);
	fprintf(cache->file_out,"------------------------\n");
}
