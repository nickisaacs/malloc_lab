#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
  /* Team name */
  "nickandmatt",
  /* First member's full name */
  "Nick Isaacs",
  /* First member's email address */
  "nicholas.isaacs@colorado.edu",
  /* Second member's full name (leave blank if none) */
  "Matt Cirbo",
  /* Second member's email address (leave blank if none) */
  "matthew.l.cirbo@colorado.edu"
};

/////////////////////////////////////////////////////////////////////////////
// Constants and macros
//
// These correspond to the material in Figure 9.43 of the text
// The macros have been turned into C++ inline functions to
// make debugging code easier.
//
/////////////////////////////////////////////////////////////////////////////
#define WSIZE       4       /* word size (bytes) */  
#define DSIZE       8       /* doubleword size (bytes) */
#define CHUNKSIZE  (1<<8)  /* initial heap size (bytes) */
#define OVERHEAD    8       /* overhead of header and footer (bytes) */

static inline int MAX(int x, int y) {
  return x > y ? x : y;
}

//
// Pack a size and allocated bit into a word
// We mask of the "alloc" field to insure only
// the lower bit is used
//
static inline size_t PACK(size_t size, int alloc) {
  return ((size) | (alloc & 0x1));
}

//
// Read and write a word at address p
//
static inline size_t GET(void *p) { return  *(size_t *)p; }
static inline void PUT( void *p, size_t val)
{
  *((size_t *)p) = val;
}

//
// Read the size and allocated fields from address p
//
static inline size_t GET_SIZE( void *p )  { 
  return GET(p) & ~0x7;
}

static inline int GET_ALLOC( void *p  ) {
  return GET(p) & 0x1;
}

//
// Given block ptr bp, compute address of its header and footer
//
static inline void *HDRP(void *bp) {
	return ( (char *)bp) - WSIZE;
}
static inline void *FTRP(void *bp) {
  return ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE);
}

//
// Given block ptr bp, compute address of next and previous blocks
//
static inline void *NEXT_BLKP(void *bp) {
  return  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)));
}

static inline void* PREV_BLKP(void *bp){
  return  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)));
}

/////////////////////////////////////////////////////////////////////////////
//
// Global Variables
//

static char *heap_listp;  /* pointer to first block */
static int *size_classp;  

//
// function prototypes for internal helper routines
//
static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
static void printblock(void *bp); 
static void checkblock(void *bp);

//
// mm_init - Initialize the memory manager 
//
int mm_init(void) 
{
  
  if ((size_classp = mem_sbrk(96)) == (void*) -1)
  	return -1;
  
  //create a list of each size class:
  PUT(size_classp, 0);		//1 - 16 bytes
  PUT(size_classp + 1, 0);	//17 - 32
  PUT(size_classp + 2, 0);	//33 - 64
  PUT(size_classp + 3, 0);	//65 - 128
  PUT(size_classp + 4, 0);	//129 - 256
  PUT(size_classp + 5, 0);	//257 - 512
  PUT(size_classp + 6, 0);	//513 - 1024
  PUT(size_classp + 7, 0);	//1025 - 2048
  PUT(size_classp + 8, 0);	//2049 - 4096
  PUT(size_classp + 9, 0);	//4096 - 8192
  PUT(size_classp + 10, 0);	//8193 - 12288
  PUT(size_classp + 11, 0);	//12289 - 16384
  PUT(size_classp + 12, 0);	//16385 - 20480
  PUT(size_classp + 13, 0);	//20481 - 24576
  PUT(size_classp + 14, 0);	//24577 - 28672
  PUT(size_classp + 15, 0);	//28673 - 32768
  PUT(size_classp + 16, 0);	//32769 - 65536
  PUT(size_classp + 17, 0);	//65537 - 131072
  PUT(size_classp + 18, 0);	//131073 - 262144
  PUT(size_classp + 19, 0);	//262145 - 524288
  PUT(size_classp + 20, 0);	//524288 - 1048576
  PUT(size_classp + 21, 0);	//1048577 - Infinity
  
  return 0;
  

  /* Create the initial empty heap*/
  /*if (( heap_listp = mem_sbrk(4*WSIZE)) == (void*) -1)
  	return -1;
  PUT(heap_listp, 0);
  PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));
  PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));
  PUT(heap_listp + (3*WSIZE), PACK(0, 1));
  heap_listp += (2*WSIZE);
  
  //Extend the empty heap with a free block of CHUNKSIZE bytes 
  if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
  	return -1;
  return 0;*/
}


//
// extend_heap - Extend heap with free block and return its block pointer
//
static void *extend_heap(size_t words) 
{
  //
  // You need to provide this
  //
  char *bp;
  size_t size;
  
  /*Allocatte an even number of words to maintain alignment */
  size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
  if ((long)(bp = mem_sbrk(size)) == -1)
  	return NULL;
  	
  /*Initialize free block header/footer and the epilogue header*/
  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
  
  /*Coalesce if the previous block was free */
  return coalesce(bp);
}


//
// Practice problem 9.8
//
// find_fit - Find a fit for a block with asize bytes 
//
static void *find_fit(size_t asize)
{
	/* First fit search */
	void *bp;
	
	for( bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
	{
		if( !GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
			return bp;
	}
	
  return NULL; /* no fit */
}

// 
// mm_free - Free a block 
//
void mm_free(void *bp)
{
  //
  // You need to provide this
  //
 
  int index = -1;
  int* freep;

    //This does the same thing as GET_SIZE, but we used a type Int pointer
    //instead of a character, makeing GET_SIZE provided not to work.
  size_t size = *((int*) bp - 2);

    //get the index for the given size.
  switch (size) {
  	case 16:
  	index = 0;
  	break;
  	case 32:
  	index = 1;
  	break;
  	case 64:
  	index = 2;
  	break;
  	case 128:
  	index = 3;
  	break;
  	case 256:
  	index = 4;
  	break;
  	case 512:
  	index = 5;
  	break;
  	case 1024:
  	index = 6;
  	break;
  	case 2048:
  	index = 7;
  	break;
  	case 4096:
  	index = 8;
  	break;
  	case 8192:
  	index = 9;
  	break;
  	case 12288:
  	index = 10;
  	break;
  	case 16384:
  	index = 11;
  	break;
  	case 20480:
  	index = 12;
  	break;
  	case 24576:
  	index = 13;
  	break;
  	case 28672:
  	index = 14;
  	break;
  	case 32768:
  	index = 15;
  	break;
  	case 65536:
  	index = 16;
  	break;
  	case 131072:
  	index = 17;
  	break;
  	case 262144:
  	index = 18;
  	break;
  	case 524288:
  	index = 19;
  	break;
  	case 1048576:
  	index = 20;
  	break;
  	default:
  	index = 21;
  	break;
  }
  
    //iterate through the free list to where the size index points:
  freep = (size_classp + index);
    
    //if value stored at that pointer is 0, there are no attached blocks
    //of that allocated size free, so the first block will point to 0
    //Else: point the newly free block to what the free list originally pointed
    //to, effectively adding it to the beginning of the list.
  if(*freep == 0) { *((int**) bp - 1) = 0; }
  else { *((int*) bp - 1) = *freep; }
    
    //lastly, point the free list to the newly free block
  *((int**) freep) = (int*) bp - 2;
  

  /*
  size_t size = GET_SIZE(HDRP(bp));
  
  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));
  coalesce(bp);*/
}

//
// coalesce - boundary tag coalescing. Return ptr to coalesced block
//
static void *coalesce(void *bp) 
{
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));
  
  if(prev_alloc && next_alloc)
  	return bp;
  
  else if(prev_alloc && !next_alloc)
  {
  	size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
  	PUT(HDRP(bp), PACK(size, 0));
  	PUT(FTRP(bp), PACK(size, 0));
  }
  
  else if(!prev_alloc && next_alloc)
  {
  	size += GET_SIZE(HDRP(PREV_BLKP(bp)));
  	PUT(FTRP(bp), PACK(size, 0));
  	PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
  	bp = PREV_BLKP(bp);
  }
  
  else
  {
  	size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
  		GET_SIZE(FTRP(NEXT_BLKP(bp)));
  	PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
  	PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
  	bp = PREV_BLKP(bp);
  }
  
  return bp;
}

//
// mm_malloc - Allocate a block with at least size bytes of payload 
//
void *mm_malloc(size_t size) 
{
  int index = 0;
  int* sizep;
  
  if(size == 0) {return NULL;}
  
  if(size <= 12288)
  {
  	if(size <= 16) { index = 0; size = 16; }
  else if(size <= 32) { index = 1; size = 32; }
  else if(size <= 64) { index = 2; size = 64; }
  else if(size <= 128) { index = 3; size = 128; }
  else if(size <= 256) { index = 4; size = 256; }
  else if(size <= 512) { index = 5; size = 512; }
  else if(size <= 1024) { index = 6; size = 1024; }
  else if(size <= 2048) { index = 7; size = 2048; }
  else if(size <= 4096) { index = 8; size = 4096; }
  else if(size <= 8192) { index = 9; size = 8192; }
  else if(size <= 12288) { index = 10; size = 12288; }
	}
	else
	{
  if(size <= 16384) { index = 11; size = 16384; }
  else if(size <= 20480) { index = 12; size = 20480; }
  else if(size <= 24576) { index = 13; size = 24576; }
  else if(size <= 28672) { index = 14; size = 28672; }
  else if(size <= 32768) { index = 15; size = 32768; }
  else if(size <= 65536) { index = 16; size = 65536; }
  else if(size <= 131072) { index = 17; size = 131072; }
  else if(size <= 262144) { index = 18; size = 262144; }
  else if(size <= 524288) { index = 19; size = 524288; }
  else if(size <= 1048576) { index = 20; size = 1048576; }
  else if(size > 1048576) { index = 21;}
	}

  if(!(*(size_classp + index))) {
  	if((sizep = mem_sbrk(size + 8)) == (void*) -1) {return NULL;}
  	PUT(sizep, size);
  }
  else {
  	sizep = (int*) *(size_classp + index);
  	*(size_classp + index) = *(sizep + 1); 
  }
  
  return (void*)((int*) sizep + 2);

  
  
  /*
  size_t asize;
  size_t extendsize;
  char *bp;
  
  if(size == 0)
  {
  	return NULL;
  }
  
  if(size <= DSIZE)
  	asize = 2*DSIZE;
  else
  	asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
  	
  if((bp = find_fit(asize)) != NULL)
  {
  	place(bp, asize);
  	return bp;
  }
  
  extendsize = MAX(asize, CHUNKSIZE);
  if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
  	return NULL;
  place(bp, asize);
  return bp;*/
} 

//
//
// Practice problem 9.9
//
// place - Place block of asize bytes at start of free block bp 
//         and split if remainder would be at least minimum block size
//
static void place(void *bp, size_t asize)
{
	size_t csize = GET_SIZE(HDRP(bp));
	
	if((csize - asize) >= (2*DSIZE))
	{
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(csize - asize, 0));
		PUT(FTRP(bp), PACK(csize - asize, 0));
	}
	else
	{
		PUT(HDRP(bp), PACK(csize, 1));
		PUT(FTRP(bp), PACK(csize, 1));
	}
}


//
// mm_realloc -- implemented for you
//
void *mm_realloc(void *ptr, size_t size)
{
	void *oldp = ptr;
	void *newp;
	size_t copy_size;
	
	copy_size = *(size_t *)((int*) oldp - 2);
	if(size > copy_size) {
		copy_size = size;
		newp = mm_malloc(size);
		if(newp == NULL) {return NULL;}
		memcpy(newp, oldp, copy_size);
		mm_free(oldp);
		return newp;
	}
	else {return oldp;}
  /*void *newp;
  size_t copySize;

  newp = mm_malloc(size);
  if (newp == NULL) {
    printf("ERROR: mm_malloc failed in mm_realloc\n");
    exit(1);
  }
  copySize = GET_SIZE(HDRP(ptr));
  if (size < copySize) {
    copySize = size;
  }
  memcpy(newp, ptr, copySize);
  mm_free(ptr);
  return newp;*/
}

//
// mm_checkheap - Check the heap for consistency 
//
void mm_checkheap(int verbose) 
{
  //
  // This provided implementation assumes you're using the structure
  // of the sample solution in the text. If not, omit this code
  // and provide your own mm_checkheap
  //
  void *bp = heap_listp;
  
  if (verbose) {
    printf("Heap (%p):\n", heap_listp);
  }

  if ((GET_SIZE(HDRP(heap_listp)) != DSIZE) || !GET_ALLOC(HDRP(heap_listp))) {
	printf("Bad prologue header\n");
  }
  checkblock(heap_listp);

  for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
    if (verbose)  {
      printblock(bp);
    }
    checkblock(bp);
  }
     
  if (verbose) {
    printblock(bp);
  }

  if ((GET_SIZE(HDRP(bp)) != 0) || !(GET_ALLOC(HDRP(bp)))) {
    printf("Bad epilogue header\n");
  }
}

static void printblock(void *bp) 
{
  size_t hsize, halloc, fsize, falloc;

  hsize = GET_SIZE(HDRP(bp));
  halloc = GET_ALLOC(HDRP(bp));  
  fsize = GET_SIZE(FTRP(bp));
  falloc = GET_ALLOC(FTRP(bp));  
    
  if (hsize == 0) {
    printf("%p: EOL\n", bp);
    return;
  }

  printf("%p: header: [%d:%c] footer: [%d:%c]\n", bp, 
	 hsize, (halloc ? 'a' : 'f'), 
	 fsize, (falloc ? 'a' : 'f')); 
}

static void checkblock(void *bp) 
{
  if ((size_t)bp % 8) {
    printf("Error: %p is not doubleword aligned\n", bp);
  }
  if (GET(HDRP(bp)) != GET(FTRP(bp))) {
    printf("Error: header does not match footer\n");
  }
}

