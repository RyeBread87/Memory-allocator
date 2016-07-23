// Partner Submission: Ryan Smith & Suzanna Kisa (Kelley)
// CS537-3 Program 3

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include "mem.h"

#define MAGIC 0xFEEDFACE

// this is only static we get to use in the lib!
static void *space;

struct global_header {
  void *alloc_space;
  int alloc_space_size;
  void *next_addr;
  int status;
  int size;
  // free list
  struct global_header *free_list_head;
} global_header;

struct used_header {
  int magic;
  int size;
};

// round up to a size that is a 
// multiple of page size
int roundPages(int sizeOfRegion){
  int sizeOfPage = getpagesize();
  if (sizeOfRegion < sizeOfPage){
    sizeOfRegion = sizeOfPage; // round up to 1 page
  }
  if (sizeOfRegion % sizeOfPage != 0){
    sizeOfRegion += (sizeOfPage - (sizeOfRegion % sizeOfPage));
  }

  fprintf(stdout,"size of page : %d\n",sizeOfPage);
  fprintf(stdout,"size of region : %d\n",sizeOfRegion);
  return sizeOfRegion;
}

// changed to return int so we could
// get the updates SizeOfRegion
int create_space(int sizeOfRegion) {
  // open the /dev/zero device
  int fd = open("/dev/zero", O_RDWR);
  
  // sizeOfRegion (in bytes) needs to be evenly divisible by the page size
  sizeOfRegion = roundPages(sizeOfRegion);
  
  void *ptr = mmap(NULL, sizeOfRegion, 
		   PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
  if (ptr == MAP_FAILED) { perror("mmap"); exit(1); }

  // close the device (don't worry, 
  // mapping should be unaffected)
  close(fd);

  space = ptr;
  return sizeOfRegion;
};

void *Mem_Init(int sizeOfRegion) {

  // create space (while rounding to the 
  // next nearest page size
  sizeOfRegion = create_space(sizeOfRegion);
  
  struct global_header *header = space;
  header->alloc_space = space + 
    sizeof(struct global_header);
  header->alloc_space_size = sizeOfRegion - 
    sizeof(struct global_header);
  header->next_addr = space;
  header->size = header->alloc_space_size;
  header->status = 1;
  header->free_list_head = header;
 
  return space;
};


// Allocate memory
void *Mem_Alloc(int size) {

  //let's start at the beginning
  struct global_header *gheader = space;

  int done=0;

  struct used_header *uheader;
  void *addr;
  void *tempaddr;
  void *tempSpace;
  int tempSpaceSize;
  int tempSize;
  int tempStatus;

  // find a block of space
  while (done == 0) {
  
    if (gheader->next_addr != NULL) {
      uheader = gheader->next_addr;
      addr = gheader->next_addr + sizeof(struct global_header);
    }
    else {
      uheader = (struct used_header *) gheader + size;
      addr = uheader + sizeof(struct global_header);
    }

    //check if we go beyond the edge of our allotted space
    if ((addr + size) > 
	(gheader->alloc_space + gheader-> alloc_space_size)) {
      return NULL;
    }

    // check if the block of space is already in use
    if (gheader->status == 2){

      tempaddr = gheader;
      tempSpace = gheader->alloc_space;
      tempSpaceSize = gheader->alloc_space_size;
      tempSize = gheader->size;

      //see if next_addr is already a block
      struct global_header *tempgheader;
      tempgheader = (struct global_header *) gheader->next_addr;

      gheader = gheader->next_addr;
      gheader->alloc_space = tempSpace;
      gheader->alloc_space_size = tempSpaceSize;
      gheader->next_addr = tempgheader + 1 + size/sizeof(struct global_header);
      continue;
    }

    gheader->size = size;
    gheader->status = 2;//"In Use";
    
    // check if need to set new free list head
    if (gheader == gheader->free_list_head){
      struct global_header *tmp = gheader->free_list_head -> next_addr;
      gheader->free_list_head = tmp;
      gheader->free_list_head -> next_addr = tmp -> next_addr;      
    }

    // found space
    done = 1;
  }

  gheader->next_addr += size + sizeof(struct global_header);

  //set up split///////////////////////////////////////////////////////////////
  struct global_header *tempgheader;
  tempgheader = (struct global_header *) gheader->next_addr;
  tempaddr = gheader;
  tempSpace = gheader->alloc_space;
  tempSpaceSize = gheader->alloc_space_size;
  tempSize = gheader->size;
  tempStatus = gheader->status;

  gheader = gheader->next_addr;
  gheader->alloc_space = tempSpace;
  gheader->alloc_space_size = tempSpaceSize;
  gheader->size = tempSize - size;
  /////////////////////////////////////////////////////////////////////////////
 
  return addr;
};


// Free the memory!
int Mem_Free(void *ptr) {
  /*
    ... USED SPACE ...
    BUFFER
    0x02987562
    ALLOCATION <-ptr
    FREE
    FREE
   */

  // actually free the space so it can be reused    
  struct global_header *gheader = space;
  gheader->status = 1; //"Free";
  gheader->next_addr = gheader->free_list_head;
  gheader->free_list_head = gheader;

  // success!
  return 0;
};

void Mem_Dump()
{
  int counter;
  struct global_header* currentBlock = space;
  printf("SPACE2: %p\n", space);
  printf("SPACE3 %p\n", currentBlock);
  //struct used_header *uheader;
  char* beginAtHeader = NULL;
  char* beginAfterHeader = NULL;
  int size;
  char* End = NULL;
  int freeSize;
  int usedSize;
  int totalSize;
  char status[5];
  //struct used_header* uheader;
  
  freeSize = 0;
  usedSize = 0;
  totalSize = 0;
  counter = 1;
  fprintf(stdout,"************************************Block list**********************************\n");
  fprintf(stdout,"No.\tStatus\tBegin\t\tEnd\t\tIn Use?\tSize\tBegin (with header)\tGSIZE\n");
  fprintf(stdout,"--------------------------------------------------------------------------------\n");
  
  while(currentBlock != NULL)
    {
      //uheader = (struct used_header*) currentBlock;
      beginAtHeader = (char*) currentBlock;
      beginAfterHeader = beginAtHeader + (int) sizeof(global_header);
      End = beginAfterHeader + currentBlock->size;
      
      //check if we're at the end
      //if ((void *)&currentBlock->status > currentBlock->alloc_space + currentBlock->alloc_space_size) {
	//return;
      //}
      // if (currentBlock->status != NULL){

        //strcpy(currentBlock->status,"Free");
      // }

      if ((currentBlock->status != 2) && (currentBlock->status != 1)){

	currentBlock->status = 1;
      }

      if (currentBlock->status==2) {  //block is in use
	strcpy(status,"Busy");
	//size = currentBlock->alloc_space_size + (int) sizeof(global_header);
	size = End - beginAfterHeader;
	usedSize = usedSize + size;
      }
      else
	{
	  //size = currentBlock->alloc_space_size + (int)sizeof(global_header);
	  size = End - beginAfterHeader; 
	  freeSize = freeSize + size;
	  strcpy(status,"Free");
	}

      fprintf(stdout,"%d\t%s\t0x%08lx\t0x%08lx\t%d\t%d\t0x%08lx\t%d\n",counter,status,(unsigned long int)beginAfterHeader,(unsigned long int)End,currentBlock->status,size,(unsigned long int)beginAtHeader,currentBlock->size);

  totalSize = totalSize + size;
  if (currentBlock > (struct global_header *) currentBlock->alloc_space + currentBlock->alloc_space_size / sizeof(struct global_header)) {
    return;
  }
  //if ((int *) &currentBlock > (int *) currentBlock->alloc_space + currentBlock->alloc_space_size / sizeof(int)) {
  //return;
  //}

  currentBlock = currentBlock->next_addr;
  counter = counter + 1;
}
  
  
  fprintf(stdout,"---------------------------------------------------------------------------------\n");
  fprintf(stdout,"*********************************************************************************\n");

  fprintf(stdout,"Total busy size = %d\n",usedSize);
  fprintf(stdout,"Total free size = %d\n",freeSize);
  fprintf(stdout,"Total size = %d\n",usedSize+freeSize);
  fprintf(stdout,"*********************************************************************************\n");
  fflush(stdout);
  return;
};

