/*
 * mm-imp.c - segregated free list + modified mm_realloc function
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
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */
#define DSIZE       8       /* Double word size (bytes) */
#define CHUNKSIZE  (1<<12)  /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y)? (x) : (y))  

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* Given block ptr bp, compute/update address of next and previous free blocks
   in the same list */
#define GET_PREV(bp)      (void *)(*(unsigned int *)(bp))
#define GET_NEXT(bp)      (void *)(*((unsigned int *)(bp) + 1))
#define SET_PREV(bp, val) (*(unsigned int *)(bp) = (unsigned int)(val))
#define SET_NEXT(bp, val) (*((unsigned int *)(bp) + 1) = (unsigned int)(val))

/* Global variables */
static char *heap_listp = 0;  /* Pointer to first block */

/* Pointers to free block lists */
static void *root_16;
static void *root_32;
static void *root_64;
static void *root_128;
static void *root_256;
static void *root_512;
static void *root_1024;
static void *root_2048;
static void *root_4096;
static void *root_etc;

/* Function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void replace(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);

static void init_free_root();
static void insert_free_block(void *bp);
static void remove_free_block(void *bp);
static void **get_free_rootp(size_t asize);
static void **get_greater_rootp(void **rootp);

static void printblock(void *bp);
static void checkblock(void *bp);
static void checkheap(int verbose);

/*
 * mm_check - check for correctness and print block list
 */
void mm_check() {
  checkheap(1);
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  init_free_root();

  /* Create the initial empty heap */
  if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
    return -1;
  PUT(heap_listp, 0);                          /* Alignment padding */
  PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* Prologue header */ 
  PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* Prologue footer */ 
  PUT(heap_listp + (3*WSIZE), PACK(0, 1));     /* Epilogue header */
  heap_listp += (2*WSIZE);

  /* Extend the empty heap with a free block of CHUNKSIZE bytes */
  if (extend_heap(CHUNKSIZE/WSIZE) == NULL) 
    return -1;
  return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
  size_t asize;      /* Adjusted block size */
  size_t extendsize; /* Amount to extend heap if no fit */
  char *bp;      

  if (heap_listp == 0){
    mm_init();
  }

  /* Ignore spurious requests */
  if (size == 0)
    return NULL;

  /* Adjust block size to include overhead and alignment reqs. */
  if (size <= DSIZE)
    asize = 2*DSIZE;
  else
    asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

  /* Search the free list for a fit */
  if ((bp = find_fit(asize)) != NULL) {
    place(bp, asize);
    return bp;
  }

  /* No fit found. Get more memory and place the block */
  extendsize = MAX(asize,CHUNKSIZE);
  if ((bp = extend_heap(extendsize/WSIZE)) == NULL)  
    return NULL;
  place(bp, asize);

  return bp;  
}

/*
 * mm_free - Free a block
 */
void mm_free(void *ptr)
{
  if (ptr == 0)
    return;

  size_t size = GET_SIZE(HDRP(ptr));

  if (heap_listp == 0){
    mm_init();
  }

  PUT(HDRP(ptr), PACK(size, 0));
  PUT(FTRP(ptr), PACK(size, 0));
  coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
  /* size_t oldsize; */
  /* void *newptr; */

  /* /\* If size == 0 then this is just free, and we return NULL. *\/ */
  /* if(size == 0) { */
  /*   mm_free(ptr); */
  /*   return 0; */
  /* } */

  /* /\* If oldptr is NULL, then this is just malloc. *\/ */
  /* if(ptr == NULL) { */
  /*   return mm_malloc(size); */
  /* } */

  /* newptr = mm_malloc(size); */

  /* /\* If realloc() fails the original block is left untouched  *\/ */
  /* if(!newptr) { */
  /*   return 0; */
  /* } */

  /* /\* Copy the old data. *\/ */
  /* oldsize = GET_SIZE(HDRP(ptr)); */
  /* if(size < oldsize) oldsize = size; */
  /* memcpy(newptr, ptr, oldsize); */

  /* /\* Free the old block. *\/ */
  /* mm_free(ptr); */

  /* return newptr; */




  void *newptr;
  size_t asize;
  size_t oldsize = GET_SIZE(HDRP(ptr));

  /* If size == 0 then this is just free, and we return NULL. */
  if(size == 0) {
    mm_free(ptr);
    return 0;
  }

  /* If oldptr is NULL, then this is just malloc. */
  if(ptr == NULL) {
    return mm_malloc(size);
  }

  if (size <= DSIZE)
    asize = 2*DSIZE;
  else
    asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
    
  if (asize <= oldsize) {
    replace(ptr, asize);
    return ptr;
  } else {
      size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
      size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(ptr)));

      if (!next_alloc && (oldsize + next_size) >= asize) {
        remove_free_block(NEXT_BLKP(ptr));
        PUT(HDRP(ptr), PACK(oldsize + next_size, 1));
        PUT(FTRP(ptr), PACK(oldsize + next_size, 1));
        return ptr;
      } else {
        newptr = mm_malloc(size);
        memcpy(newptr, ptr, oldsize);
        mm_free(ptr);
        return newptr;
      }
    }
}

/* 
 * The remaining routines are internal helper routines 
 */

/* 
 * extend_heap - Extend heap with free block and return its block pointer
 */
static void *extend_heap(size_t words) 
{
  char *bp;
  size_t size;

  /* Allocate an even number of words to maintain alignment */
  size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
  if ((long)(bp = mem_sbrk(size)) == -1)
    return NULL;

  /* Initialize free block header/footer and the epilogue header */
  PUT(HDRP(bp), PACK(size, 0));         /* Free block header */
  PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

  /* Coalesce if the previous block was free */
  return coalesce(bp);
}

/* 
 * place - Place block of asize bytes at start of free block bp 
 *         and split if remainder would be at least minimum block size
 */
static void place(void *bp, size_t asize)
{
  size_t csize = GET_SIZE(HDRP(bp));
  remove_free_block(bp);

  if ((csize - asize) >= (2*DSIZE)) {
    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));
    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(csize-asize, 0));
    PUT(FTRP(bp), PACK(csize-asize, 0));
    insert_free_block(bp);
  }
  else {
    PUT(HDRP(bp), PACK(csize, 1));
    PUT(FTRP(bp), PACK(csize, 1));
  }
}

static void replace(void *bp, size_t asize) {
  size_t csize = GET_SIZE(HDRP(bp));

  if ((csize - asize) >= (2*DSIZE)) {
    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));
    bp = NEXT_BLKP(bp);
    PUT(HDRP(bp), PACK(csize-asize, 0));
    PUT(FTRP(bp), PACK(csize-asize, 0));
    insert_free_block(bp);
  }
  else {
    PUT(HDRP(bp), PACK(csize, 1));
    PUT(FTRP(bp), PACK(csize, 1));
  }
}

/* 
 * find_fit - Find a fit for a block with asize bytes 
 */
static void *find_fit(size_t asize)
{
  void **rootp;
  void *bp;

  rootp = get_free_rootp(asize);
  while (rootp) {
    // first fit search
    bp = *rootp;
    while (bp) {
      if (asize <= GET_SIZE(HDRP(bp))) return bp;
      bp = GET_NEXT(bp);
    }
    rootp = get_greater_rootp(rootp);
  }

  return NULL; /* No fit */
}

/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 */
static void *coalesce(void *bp) 
{
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));

  if (prev_alloc && next_alloc) {            /* Case 1 */
    insert_free_block(bp);
    return bp;
  }

  else if (prev_alloc && !next_alloc) {      /* Case 2 */
    remove_free_block(NEXT_BLKP(bp));

    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size,0));

    insert_free_block(bp);
  }

  else if (!prev_alloc && next_alloc) {      /* Case 3 */
    remove_free_block(PREV_BLKP(bp));

    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);

    insert_free_block(bp);
  }

  else {                                     /* Case 4 */
    remove_free_block(NEXT_BLKP(bp));
    remove_free_block(PREV_BLKP(bp));

    size += GET_SIZE(HDRP(PREV_BLKP(bp))) + 
      GET_SIZE(FTRP(NEXT_BLKP(bp)));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);

    insert_free_block(bp);
  }
  return bp;
}

/*
 * Free list/tree operating functions
 */
static void init_free_root() {
  root_16 = 0;
  root_32 = 0;
  root_64 = 0;
  root_128 = 0;
  root_256 = 0;
  root_512 = 0;
  root_1024 = 0;
  root_2048 = 0;
  root_4096 = 0;
  root_etc = 0;
}

static void insert_free_block(void *bp) {
  void **rootp = get_free_rootp(GET_SIZE(HDRP(bp)));

  if (*rootp) SET_PREV(*rootp, bp);
  SET_PREV(bp, NULL);
  SET_NEXT(bp, *rootp);
  *rootp = bp;
}

static void remove_free_block(void *bp) {
  void **rootp = get_free_rootp(GET_SIZE(HDRP(bp)));
  void *prev = GET_PREV(bp);
  void *next = GET_NEXT(bp);

  if (prev) SET_NEXT(prev, next);
  else *rootp = next;
  if (next) SET_PREV(next, prev);
}

static void **get_free_rootp(size_t asize) {
  if (asize <= 16) return &root_16;
  else if (asize <= 32) return &root_32;
  else if (asize <= 64) return &root_64;
  else if (asize <= 128) return &root_128;
  else if (asize <= 256) return &root_256;
  else if (asize <= 512) return &root_512;
  else if (asize <= 1024) return &root_1024;
  else if (asize <= 2048) return &root_2048;
  else if (asize <= 4096) return &root_4096;
  else return &root_etc;
}

static void **get_greater_rootp(void **rootp) {
  if (rootp == &root_16) return &root_32;
  else if (rootp == &root_32) return &root_64;
  else if (rootp == &root_64) return &root_128;
  else if (rootp == &root_128) return &root_256;
  else if (rootp == &root_256) return &root_512;
  else if (rootp == &root_512) return &root_1024;
  else if (rootp == &root_1024) return &root_2048;
  else if (rootp == &root_2048) return &root_4096;
  else if (rootp == &root_4096) return &root_etc;
  else return NULL;
}

/*
 * debug helper functions
 */
static void printblock(void *bp) 
{
  size_t hsize, halloc, fsize, falloc;

  checkheap(0);
  hsize = GET_SIZE(HDRP(bp));
  halloc = GET_ALLOC(HDRP(bp));  
  fsize = GET_SIZE(FTRP(bp));
  falloc = GET_ALLOC(FTRP(bp));  

  if (hsize == 0) {
    printf("%p: EOL\n", bp);
    return;
  }

  printf("%p: header: [%ld:%c] footer: [%ld:%c]\n", bp, 
         (long int)hsize, (halloc ? 'a' : 'f'), 
         (long int)fsize, (falloc ? 'a' : 'f')); 
}

static void checkblock(void *bp) 
{
  if ((size_t)bp % 8)
    printf("Error: %p is not doubleword aligned\n", bp);
  if (GET(HDRP(bp)) != GET(FTRP(bp)))
    printf("Error: header does not match footer\n");
}

/* 
 * checkheap - Minimal check of the heap for consistency 
 */
void checkheap(int verbose) 
{
  char *bp = heap_listp;

  if (verbose)
    printf("Heap (%p):\n", heap_listp);

  if ((GET_SIZE(HDRP(heap_listp)) != DSIZE) || !GET_ALLOC(HDRP(heap_listp)))
    printf("Bad prologue header\n");
  checkblock(heap_listp);

  for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
    if (verbose) 
      printblock(bp);
    checkblock(bp);
  }

  if (verbose)
    printblock(bp);
  if ((GET_SIZE(HDRP(bp)) != 0) || !(GET_ALLOC(HDRP(bp))))
    printf("Bad epilogue header\n");
}
