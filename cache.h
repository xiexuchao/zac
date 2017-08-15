#ifndef _CACHE_H
#define _CACHE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SUCCESS		0
#define FAILURE		1
#define READ		0
#define WRITE		1
#define CLEAN		0
#define DIRTY		1
#define SIZE_BUF	256

//for NSW-Cache
struct cache_info{
	unsigned int size_block;
	unsigned int size_cache;
	
	unsigned int blk_trc_all;	//trace footprint
	
	unsigned int blk_max_all;	//cache capacity
	unsigned int blk_max_reg;	//regular cache
	unsigned int blk_max_evt;	//evicting cache
	unsigned int blk_max_gst;	//ghost cache
	
	unsigned int blk_now_reg;
	unsigned int blk_now_evt;
	unsigned int blk_now_gst;
	
	unsigned int set_now_evt;	//current sets in evicting cache
	unsigned int set_size[30000];
	
	unsigned int hit_red_reg;//cache hit times (blocks)
	unsigned int hit_wrt_reg;
	unsigned int hit_red_evt; //must be 0 ?
	unsigned int hit_wrt_evt;
	unsigned int hit_red_gst;
	unsigned int hit_wrt_gst;
	
	
	struct req_info *req;
	struct blk_info *blk_head_reg;
	struct blk_info *blk_tail_reg;
	struct blk_info *blk_head_gst;
	struct blk_info *blk_tail_gst;
	struct blk_info *blk_head_evt;
	struct blk_info *blk_tail_evt;
	struct set_info *set_head_evt;
	struct set_info *set_tail_evt;
	
	char buffer[SIZE_BUF];
	char filename_trc[128];
	char filename_out[128];
	char filename_smr[128];
    char filename_ssd[128];
    
    FILE *file_trc;
    FILE *file_out;
    FILE *file_smr;
    FILE *file_ssd;
};

struct req_info{
	long long time;
    long long blkn;
    unsigned int type;				//0->Read,1->Write
    unsigned int size;
};

struct set_info{
	unsigned int setn;
	unsigned int size;
	//unsigned int priority;
	struct blk_info *blk_head;
	struct blk_info *blk_tail;
	struct set_info *set_prev;
	struct set_info *set_next;
};

struct blk_info{
	unsigned int blkn;	//block number
	unsigned int setn;	// set number
	unsigned int state; // clean or dirty
	struct blk_info *blk_prev;
	struct blk_info *blk_next;
};

//cache.c
void cache_run_lru(char *trace, char *output, char *smrTrc, char *ssdTrc);
void cache_run_larc(char *trace, char *output, char *smrTrc, char *ssdTrc);
void cache_run_zac(char *trace, char *output, char *smrTrc, char *ssdTrc);
void cache_delete_tail_blk_reg(struct cache_info *cache);
void cache_delete_tail_blk_gst(struct cache_info *cache);
void cache_delete_tail_set_evt(struct cache_info *cache);
void cache_free(struct cache_info *cache);
int get_req(struct cache_info *cache);
void alloc_assert(void *p,char *s);

//lru.c
void cache_init_lru(struct cache_info *cache,char *trace, char *output, char *smrTrc, char *ssdTrc);
void cache_lru(struct cache_info *cache);
int cache_blk_lru(struct cache_info *cache,unsigned int blkn,unsigned int state);
void cache_print_lru(struct cache_info *cache);

//larc
void cache_init_larc(struct cache_info *cache,char *trace, char *output, char *smrTrc, char *ssdTrc);
void cache_larc(struct cache_info *cache);
int cache_blk_larc_reg(struct cache_info *cache,unsigned int blkn,unsigned int state);
int cache_blk_larc_gst(struct cache_info *cache,unsigned int blkn,unsigned int state);
void cache_print_larc(struct cache_info *cache);

//zac
void cache_init_zac(struct cache_info *cache,char *trace, char *output, char *smrTrc, char *ssdTrc);
void zac_delete_tail_blk_reg(struct cache_info *cache);
void zac_delete_tail_blk_gst(struct cache_info *cache);
int zac_dedupe_blk_gst(struct cache_info *cache,unsigned int blkn);
void zac_delete_tail_set_evt(struct cache_info *cache,unsigned int setn);
int zac_find_max(struct cache_info* cache);
void cache_zac(struct cache_info *cache);
int cache_blk_zac_reg(struct cache_info *cache,unsigned int blkn,unsigned int state);
int cache_blk_zac_evt(struct cache_info *cache,unsigned int blkn,unsigned int state);
int cache_blk_zac_gst(struct cache_info *cache,unsigned int blkn,unsigned int state);
void cache_print_zac(struct cache_info *cache);






#endif
