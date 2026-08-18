/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "team xlcx",
    /* First member's full name */
    "xueluochenxin",
    /* First member's email address */
    "xueluochenxin666@gamail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))//对齐用的

//my define
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)
#define MAX(x,y) ((x)>(y)?(x):(y))
#define PACK(size,alloc) ((size)|(alloc))
#define GET(p) (*(unsigned int*)(p))
#define PUT(p,val) (*(unsigned int *)(p)=(val))
#define GET_SIZE(p) (GET(p)& ~0x7)  //getsize得到的是payload+head+foot的size
#define GET_ALLOC(p) (GET(p) & 0x1) //1代表已经分配了

#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char*)(bp)+GET_SIZE((char*)(bp)-WSIZE))//指向下一块的payload
#define PREV_BLKP(bp) ((char*)(bp)-GET_SIZE((char*)(bp)-DSIZE))//指向前一块的payload


//变量声明
static char* heap_listp; //指向堆中第一个空闲块的指针 位置是序言块的foot

static void *extend_heap(size_t words);
static void* coalesce(char* bp);
static void* find_fit(size_t asize);
static void place(void* bp, size_t asize);






/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if((heap_listp=mem_sbrk(4*WSIZE))==(void*) -1)
    {
        return -1;
    }
    PUT(heap_listp,0);
    PUT(heap_listp+WSIZE,PACK(DSIZE,1));//序言块头
    PUT(heap_listp+DSIZE,PACK(DSIZE,1));//序言块foot
    PUT(heap_listp+3*WSIZE,PACK(0,1));//结尾块 大小0 a=1
    heap_listp+=(2*WSIZE);

    if(extend_heap(CHUNKSIZE/WSIZE)==NULL)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}
static void *extend_heap(size_t words)//words含义是带有多少个WSIZE
{
   char *bp;
   size_t size;

   size=(words%2)?(words+1)*WSIZE:words*WSIZE;
   
   if((bp=mem_sbrk(size))==(void *)-1)
   {
    return NULL;
   }
   
   PUT(HDRP(bp),PACK(size,0));
   PUT(FTRP(bp),PACK(size,0));
   PUT(HDRP(NEXT_BLKP(bp)),PACK(0,1));

   return coalesce(bp);//合并后首地址可能变了，所以我们必须返回合并后的首地址
}
static void* coalesce(char* bp)
{
  size_t prev_alloc= GET_ALLOC(HDRP(PREV_BLKP(bp)));
  size_t next_alloc= GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size=GET_SIZE(HDRP(bp));

  if(prev_alloc&&next_alloc)//前后都已分配
  {
    return bp;
  }
  else if(prev_alloc&&!next_alloc)//前分配 后空闲
  {
    size+= GET_SIZE(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(bp),PACK(size,0));//先修改head，这样后面用FTRP得到的foot就是合并块的foot
    PUT(FTRP(bp),PACK(size,0));
  }
  else if(!prev_alloc&&next_alloc)//前空闲，后分配
  {
    size+=GET_SIZE(FTRP(PREV_BLKP(bp)));
    PUT(FTRP(bp),PACK(size,0));
    PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
    bp=PREV_BLKP(bp);
  }
  else if(!prev_alloc&&!next_alloc)//前后都空闲
  {
     size+=GET_SIZE(HDRP(NEXT_BLKP(bp)))+GET_SIZE(FTRP(PREV_BLKP(bp)));
     PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
     PUT(FTRP(NEXT_BLKP(bp)),PACK(size,0));
     bp=PREV_BLKP(bp);
  }
  return bp;
}
/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
   if(size==0)
   {
    return NULL;
   }
   size_t asize;
   size_t expandsize;
   char *bp;
   
   if(size<DSIZE)
   {
    asize=2*DSIZE;
   }
   else
   {
    asize=DSIZE*((size+(DSIZE)+(DSIZE-1))/DSIZE);
   }

   //查找
   if((bp=find_fit(asize))!=NULL) //首次适配
   {
    place(bp,asize);
    return bp;
   }
   //没找到
   expandsize= MAX(asize,CHUNKSIZE);
   if((bp=extend_heap(expandsize/WSIZE) )==NULL)
   {
    return NULL;
   }
   place(bp,asize);
   return bp;
}
static void* find_fit(size_t asize) //首次适配
{
    char *ptr=heap_listp;
    for(ptr;GET_SIZE(HDRP(ptr))>0;ptr=NEXT_BLKP(ptr))
    {
        if( asize<=GET_SIZE(HDRP(ptr)) && !GET_ALLOC(HDRP(ptr)))
        {
            return ptr;
        }
    }
    return NULL;
}
static void place(void* bp,size_t asize)
{
  size_t size=GET_SIZE(HDRP(bp));
  if(size-asize>=2*DSIZE)//给剩余块放置头部和尾部
  {
     PUT(HDRP(bp),PACK(asize,1));
     PUT(FTRP(bp),PACK(asize,1));

     bp=NEXT_BLKP(bp);
     PUT(HDRP(bp),PACK(size-asize,0));
     PUT(FTRP(bp),PACK(size-asize,0));
  }
  else//全分配给当前的指针，作为一个大块
  {
      PUT(HDRP(bp),PACK(size,1));
      PUT(FTRP(bp),PACK(size,1));
  }
}
/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    if(ptr==NULL) return;
    size_t size=GET_SIZE(HDRP(ptr));
    
    PUT(HDRP(ptr),PACK(size,0));
    PUT(FTRP(ptr),PACK(size,0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    size_t oldsize;
    if(size==0)
    {
        mm_free(ptr);
        return NULL;
    }
    if(ptr==NULL)
    {
        return mm_malloc(size);
    }
    oldsize=GET_SIZE(HDRP(ptr));
    size_t oldpayload=oldsize-DSIZE;
    size_t asize;
    if(size<DSIZE)
    {
     asize=2*DSIZE;
    }
    else
    {
     asize=DSIZE*((size+(DSIZE)+(DSIZE-1))/DSIZE);
    }
    if(oldpayload>=asize)
    {
        return ptr;
    }
    else
    {
        int flag=GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
        size_t nextsize=GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        //如果下一块是空闲的且和当前块合并得到的新payload大于asize
        if(!flag&&nextsize+oldsize-DSIZE>=size)
        {
            size_t new_size=nextsize+oldsize;
            PUT(HDRP(ptr), PACK(new_size, 1));
            PUT(FTRP(ptr), PACK(new_size, 1));
            return ptr;
        }
        //无法合并，就重新malloc后复制数据过去
        else
        {
            char * newptr=(char*)mm_malloc(size);
            if(newptr==NULL)
            {
                return NULL;
            }
            else
            {
                memcpy(newptr,ptr,oldpayload);
                mm_free(ptr);
                return newptr;
            }
        }
    }
}














