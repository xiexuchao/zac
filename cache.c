#include "cache.h"

void cache_run_lru(char *trace, char *output, char *smrTrc, char *ssdTrc)
{
	int i=0;
	struct cache_info *cache;
	
	cache=(struct cache_info *)malloc(sizeof(struct cache_info));
	alloc_assert(cache,"cache");
	memset(cache,0,sizeof(struct cache_info));
	
	cache_init_lru(cache,trace,output,smrTrc,ssdTrc);
	while(get_req(cache) != FAILURE)
	{
		cache_lru(cache);
		
		i++;
		if(i%100000==0)
		{
			printf("LRU is Handling %d \n",i);
		}
	}
	cache_print_lru(cache);
	cache_free(cache);
}

void cache_run_larc(char *trace, char *output, char *smrTrc, char *ssdTrc)
{
	int i=0;
	struct cache_info *cache;
	
	cache=(struct cache_info *)malloc(sizeof(struct cache_info));
	alloc_assert(cache,"cache");
	memset(cache,0,sizeof(struct cache_info));
	
	cache_init_larc(cache,trace,output,smrTrc,ssdTrc);
	while(get_req(cache) != FAILURE)
	{
		cache_larc(cache);
		
		i++;
		if(i%100000==0)
		{
			printf("LARC is Handling %d \n",i);
		}
	}
	cache_print_larc(cache);
	cache_free(cache);
}


void cache_run_zac(char *trace, char *output, char *smrTrc, char *ssdTrc)
{
	int i=0;
	struct cache_info *cache;
	
	cache=(struct cache_info *)malloc(sizeof(struct cache_info));
	alloc_assert(cache,"cache");
	memset(cache,0,sizeof(struct cache_info));
	
	cache_init_zac(cache,trace,output,smrTrc,ssdTrc);
	while(get_req(cache) != FAILURE)
	{
		cache_zac(cache);
		
		i++;
		if(i%100000==0)
		{
			printf("ZAC is Handling %d \n",i);
		}
	}
	cache_print_zac(cache);
	cache_free(cache);
}

void cache_delete_tail_blk_reg(struct cache_info *cache)
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
		//A dirty blocks is evicted
		
	}
	free(block);
}

void cache_delete_tail_blk_gst(struct cache_info *cache)
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
	if(block->state == DIRTY)
	{
		//A dirty blocks is evicted
		
	}
	free(block);
}

void cache_delete_tail_set_evt(struct cache_info *cache)
{
	
}


void cache_free(struct cache_info *cache)
{
	struct blk_info *index_reg,*blk_reg;
	struct blk_info *index_gst,*blk_gst;
	struct set_info *index_evt,*set_evt;
	
	index_reg=cache->blk_head_reg;
	while(index_reg)
	{
		blk_reg=index_reg;
		index_reg=index_reg->blk_next;
		free(blk_reg);
	}
	
	index_gst=cache->blk_head_gst;
	while(index_gst)
	{
		blk_gst=index_gst;
		index_gst=index_gst->blk_next;
		free(blk_gst);
	}
	
	index_evt=cache->set_head_evt;
	while(index_evt)
	{
		set_evt=index_evt;
		index_evt=index_evt->set_next;
		free(set_evt);
	}
	
	fclose(cache->file_trc);
	fclose(cache->file_out);
	fclose(cache->file_smr);
	fclose(cache->file_ssd);
	
	free(cache->req);
	free(cache);
}

int get_req(struct cache_info *cache)
{
	long long blkn;
	unsigned int size;
	unsigned int type;
	
	fgets(cache->buffer,SIZE_BUF,cache->file_trc);
	if(feof(cache->file_trc))
	{
		printf("***Read <%s> end***\n",cache->filename_trc);
		return FAILURE;
	}	
	
	sscanf(cache->buffer,"%lld %d %d\n",&blkn,&size,&type);
	//printf("%lld %d %d\n",blkn,size,type);
	cache->req->type=type;
	cache->req->blkn=blkn;
	cache->req->size=size;

	return SUCCESS;
}

void alloc_assert(void *p,char *s)
{
	if(p!=NULL)
	{
		return;
	}
	printf("malloc %s error\n",s);
	getchar();
	exit(-1);
}
