#include "cache.h"

void cache_run_lru(char *trace, char *output, char *smrTrc, char *ssdTrc, unsigned int ssdsize)
{
	int i=0;
	struct cache_info *cache;
	
	cache=(struct cache_info *)malloc(sizeof(struct cache_info));
	cache_alloc_assert(cache,"cache");
	memset(cache,0,sizeof(struct cache_info));
	
	lru_init(cache,trace,output,smrTrc,ssdTrc,ssdsize);
	while(cache_get_req(cache) != FAILURE)
	{
		lru_main(cache);
		
		i++;
		if(i%100000==0)
		{
			printf("LRU is Handling %d %s ssdsize=%d \n",i,trace,ssdsize);
		}
	}
	lru_print(cache);
	cache_free(cache);
}

void cache_run_larc(char *trace, char *output, char *smrTrc, char *ssdTrc, unsigned int ssdsize)
{
	int i=0;
	struct cache_info *cache;
	
	cache=(struct cache_info *)malloc(sizeof(struct cache_info));
	cache_alloc_assert(cache,"cache");
	memset(cache,0,sizeof(struct cache_info));
	
	larc_init(cache,trace,output,smrTrc,ssdTrc);
	while(cache_get_req(cache) != FAILURE)
	{
		larc_main(cache);
		
		i++;
		if(i%100000==0)
		{
			printf("LARC is Handling %d %s ssdsize=%d \n",i,trace,ssdsize);
		}
	}
	larc_print(cache);
	cache_free(cache);
}


void cache_run_zac(char *trace, char *output, char *smrTrc, char *ssdTrc, unsigned int ssdsize)
{
	int i=0;
	struct cache_info *cache;
	
	cache=(struct cache_info *)malloc(sizeof(struct cache_info));
	cache_alloc_assert(cache,"cache");
	memset(cache,0,sizeof(struct cache_info));
	
	zac_init(cache,trace,output,smrTrc,ssdTrc);
	while(cache_get_req(cache) != FAILURE)
	{
		zac_main(cache);
		
		i++;
		if(i%100000==0)
		{
			printf("ZAC is Handling %d %s ssdsize=%d \n",i,trace,ssdsize);
		}
	}
	zac_print(cache);
	cache_free(cache);
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

int cache_get_req(struct cache_info *cache)
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
	
	cache->blk_trc_all += size;
	if(type == READ)
	{
		cache->blk_trc_red += cache->req->size;
	}
	else
	{
		cache->blk_trc_wrt += cache->req->size;
	}	

	return SUCCESS;
}

void cache_alloc_assert(void *p,char *s)
{
	if(p!=NULL)
	{
		return;
	}
	printf("malloc %s error\n",s);
	getchar();
	exit(-1);
}
