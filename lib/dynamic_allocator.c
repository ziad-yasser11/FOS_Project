/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"


//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//=====================================================
// 1) GET BLOCK SIZE (including size of its meta data):
//=====================================================
__inline__ uint32 get_block_size(void* va)
{
	uint32 *curBlkMetaData = ((uint32 *)va - 1) ;
	return (*curBlkMetaData) & ~(0x1);
}

//===========================
// 2) GET BLOCK STATUS:
//===========================
__inline__ int8 is_free_block(void* va)
{
	uint32 *curBlkMetaData = ((uint32 *)va - 1) ;
	return (~(*curBlkMetaData) & 0x1) ;
}

//===========================
// 3) ALLOCATE BLOCK:
//===========================

void *alloc_block(uint32 size, int ALLOC_STRATEGY)
{
	void *va = NULL;
	switch (ALLOC_STRATEGY)
	{
	case DA_FF:
		va = alloc_block_FF(size);
		break;
	case DA_NF:
		va = alloc_block_NF(size);
		break;
	case DA_BF:
		va = alloc_block_BF(size);
		break;
	case DA_WF:
		va = alloc_block_WF(size);
		break;
	default:
		cprintf("Invalid allocation strategy\n");
		break;
	}
	return va;
}

//===========================
// 4) PRINT BLOCKS LIST:
//===========================

void print_blocks_list(struct MemBlock_LIST list)
{
	cprintf("=========================================\n");
	struct BlockElement* blk ;
	cprintf("\nDynAlloc Blocks List:\n");
	LIST_FOREACH(blk, &list)
	{
		cprintf("(size: %d, isFree: %d,add:%x)\n", get_block_size(blk), is_free_block(blk),blk) ;
	}
	cprintf("=========================================\n");

}
//
////********************************************************************************//
////********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

bool is_initialized = 0;
//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================
void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{
	//==================================================================================
	//DON'T CHANGE THESE LINES==========================================================
	//==================================================================================
	{
		if (initSizeOfAllocatedSpace % 2 != 0) initSizeOfAllocatedSpace++; //ensure it's multiple of 2
		if (initSizeOfAllocatedSpace == 0)
			return ;
		is_initialized = 1;
	}
	//==================================================================================
	//==================================================================================

	//TODO: [PROJECT'24.MS1 - #04] [3] DYNAMIC ALLOCATOR - initialize_dynamic_allocator
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("initialize_dynamic_allocator is not implemented yet");
	//Your Code is Here...
	LIST_INIT(&freeBlocksList);
	uint32 *BEGBlock=(uint32 *)daStart;
	*BEGBlock = 0;
	*BEGBlock |= 1;
	uint32 *ENDBlock=(uint32 *)(daStart+initSizeOfAllocatedSpace-sizeof(int));
	*ENDBlock = 0;
	*ENDBlock |= 1;
	struct BlockElement *first_free_block=(struct BlockElement *)(daStart+2*sizeof(int));
	(*first_free_block).prev_next_info.le_next = NULL;
	(*first_free_block).prev_next_info.le_prev = NULL;
	uint32 *header_pt = (uint32 *)(daStart+sizeof(int));
	uint32 *footer_pt =(uint32 *)(daStart+initSizeOfAllocatedSpace-2*sizeof(int));
	*header_pt = initSizeOfAllocatedSpace-2*sizeof(int);
	*footer_pt = initSizeOfAllocatedSpace-2*sizeof(int);
	*header_pt=*header_pt & ~1;
	*footer_pt=*footer_pt & ~1;
	LIST_INSERT_HEAD(&freeBlocksList, first_free_block);
}
//==================================
// [2] SET BLOCK HEADER & FOOTER:
//==================================
void set_block_data(void* va, uint32 totalSize, bool isAllocated)
{
	//TODO: [PROJECT'24.MS1 - #05] [3] DYNAMIC ALLOCATOR - set_block_data
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("set_block_data is not implemented yet");
	//Your Code is Here...
	uint32 *header_pt = (uint32 *)(va - sizeof(int));
	uint32 *footer_pt = (uint32 *)((uint32)va+totalSize-2*sizeof(int));
	*header_pt=totalSize;
	*footer_pt=totalSize;
	if(isAllocated){
		    *header_pt=*header_pt|1;
		    *footer_pt=*footer_pt |1;
		}
	else
	{
		*header_pt=*header_pt & ~1;
		*footer_pt=*footer_pt & ~1;
	}
}

//=========================================
// [3] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *alloc_block_FF(uint32 size)
{
	//==================================================================================
	//DON'T CHANGE THESE LINES==========================================================
	//==================================================================================
	{
		if (size % 2 != 0) size++;	//ensure that the size is even (to use LSB as allocation flag)
		if (size < DYN_ALLOC_MIN_BLOCK_SIZE)
			size = DYN_ALLOC_MIN_BLOCK_SIZE ;
		if (!is_initialized)
		{
			uint32 required_size = size + 2*sizeof(int) /*header & footer*/ + 2*sizeof(int) /*da begin & end*/ ;
			uint32 da_start = (uint32)sbrk(ROUNDUP(required_size, PAGE_SIZE)/PAGE_SIZE);
			uint32 da_break = (uint32)sbrk(0);
			initialize_dynamic_allocator(da_start, da_break - da_start);
		}
	}
	//==================================================================================
	//==================================================================================

	//TODO: [PROJECT'24.MS1 - #06] [3] DYNAMIC ALLOCATOR - alloc_block_FF
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("alloc_block_FF is not implemented yet");
	//Your Code is Here...
	uint32 total_size =(uint32) size + 2*sizeof(int);
	if(size == 0 || total_size < 16)
		return NULL;
	bool block_found = 0;
	struct BlockElement * block;
	LIST_FOREACH(block,&freeBlocksList)
	{
		if(get_block_size(block)>=total_size)
		{
			block_found = 1;
			break;
		}
	}
	if(block_found)
	{
		if(get_block_size(block)-total_size>=16)
		{
			// check size
			struct BlockElement *newBlock = (struct BlockElement *)((uint32)block+total_size);
			uint32 new_block_size = (get_block_size(block) - total_size);
			set_block_data(newBlock,new_block_size,0);
		    LIST_INSERT_AFTER(&freeBlocksList, block,newBlock);
		    LIST_REMOVE(&freeBlocksList, block);
			set_block_data(block,total_size,1);
		}
		else
		{
			set_block_data(block,get_block_size(block),1);
			LIST_REMOVE(&freeBlocksList, block);
		}
		return block;
	}
	else
	{
		void * ret = sbrk(1);
		if(ret == (void *)-1)
		{
			return NULL;
		}
		else
		{
			/*uint32 * EndBlock = (uint32 *)((uint32)(ret) + PAGE_SIZE - sizeof(int));
			*EndBlock = 0;
			*EndBlock |= 1;*/
			uint32 * lastBlockFooter =(uint32 *)(ret - 2*sizeof(int));
			// last block is allocated
			if((*lastBlockFooter) & 1)
			{
				struct BlockElement *newBlock = (struct BlockElement *)(ret);
				set_block_data(newBlock,PAGE_SIZE,0);
				// is block inserted sorted ??
				LIST_INSERT_TAIL(&freeBlocksList,newBlock);
			}// last block is free (Merge)
			else
			{
				struct BlockElement *lastBlock = (struct BlockElement *)((uint32)lastBlockFooter-(*lastBlockFooter)+2*sizeof(int));
				set_block_data(lastBlock,(*lastBlockFooter+PAGE_SIZE),0);
			}
			return alloc_block_FF(size);
		}
	}
}
//=========================================
// [4] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void *alloc_block_BF(uint32 size)
{
	//TODO: [PROJECT'24.MS1 - BONUS] [3] DYNAMIC ALLOCATOR - alloc_block_BF
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("alloc_block_BF is not implemented yet");
	//Your Code is Here...
	{
			if (size % 2 != 0) size++;	//ensure that the size is even (to use LSB as allocation flag)
			if (size < DYN_ALLOC_MIN_BLOCK_SIZE)
				size = DYN_ALLOC_MIN_BLOCK_SIZE ;
			if (!is_initialized)
			{
				uint32 required_size = size + 2*sizeof(int) /*header & footer*/ + 2*sizeof(int) /*da begin & end*/ ;
				uint32 da_start = (uint32)sbrk(ROUNDUP(required_size, PAGE_SIZE)/PAGE_SIZE);
				uint32 da_break = (uint32)sbrk(0);
				initialize_dynamic_allocator(da_start, da_break - da_start);
			}
		}
	uint32 total_size = size + 2*sizeof(int);
		if(size == 0 || total_size < 16)
			return NULL;
		bool equal_found = 0 , large_found =0;
		struct BlockElement * block;
		struct BlockElement * BFblock;
		uint32 min = UINT_MAX;
		LIST_FOREACH(block,&freeBlocksList)
		{
			int diff = get_block_size(block)-total_size;
			if(diff == 0)
			{
				equal_found = 1;
				break;
			}
			else if(diff > 0 && diff < min)
			{
				large_found = 1;
				min = diff;
				BFblock = block ;
			}
		}
		if(equal_found)
		{
				set_block_data(block,total_size,1);
				LIST_REMOVE(&freeBlocksList, block);
				return block;
		}
		else if(large_found)
		{
			if(get_block_size(BFblock)-total_size>=16)
			{
				// check size
				struct BlockElement *newBlock = (struct BlockElement *)((uint32)BFblock+total_size);
				uint32 new_block_size = (get_block_size(BFblock) - total_size);
				set_block_data(newBlock,new_block_size,0);
				LIST_INSERT_AFTER(&freeBlocksList, BFblock,newBlock);
				LIST_REMOVE(&freeBlocksList, BFblock);
				set_block_data(BFblock,total_size,1);
				return BFblock;
			}
			else
			{
				set_block_data(BFblock,get_block_size(BFblock),1);
				LIST_REMOVE(&freeBlocksList, BFblock);
				return BFblock;
			}
		}
		else
		{
			void * ret = sbrk(1);
			if(ret == (void *)-1)
			{
				return NULL;
			}
			else
			{
				/*uint32 * EndBlock = (uint32 *)((uint32)(ret) + PAGE_SIZE - sizeof(int));
				*EndBlock = 0;
				*EndBlock |= 1;*/
				uint32 * lastBlockFooter =(uint32 *)(ret - 2*sizeof(int));
				// last block is allocated
				if((*lastBlockFooter) & 1)
				{
					struct BlockElement *newBlock = (struct BlockElement *)(ret);
					set_block_data(newBlock,PAGE_SIZE,0);
					// is block inserted sorted ??
					LIST_INSERT_TAIL(&freeBlocksList,newBlock);
				}// last block is free (Merge)
				else
				{
					struct BlockElement *lastBlock = (struct BlockElement *)((uint32)lastBlockFooter-(*lastBlockFooter)+2*sizeof(int));
					set_block_data(lastBlock,(*lastBlockFooter+PAGE_SIZE),0);
				}
				return alloc_block_BF(size);
			}
		}
}

//===================================================
// [5] FREE BLOCK WITH COALESCING:
//===================================================
void free_block(void *va)
{
	//TODO: [PROJECT'24.MS1 - #07] [3] DYNAMIC ALLOCATOR - free_block
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("free_block is not implemented yet");
	//Your Code is Here...
	if(va == NULL)
		return;
	struct BlockElement * block = va;
	uint32 *header_pt = (uint32 *)(va-sizeof(int));
	uint32 *footer_pt =(uint32 *)((uint32)va+get_block_size(va)-2*sizeof(int));
	// check if block is free
	if(!(*header_pt & 1))
		return;
	*header_pt=*header_pt & ~1;
	*footer_pt=*footer_pt & ~1;
	uint32 *previous_footer_pt =(uint32 *)(va-2*sizeof(int));
	uint32 *next_header_pt = (uint32 *)((uint32)va+get_block_size(va)-sizeof(int));
	// case both previous and next are free
	if((!(*previous_footer_pt & 1)) && (!(*next_header_pt & 1)))
	{
		struct BlockElement *previousBlock =(struct BlockElement *) ((uint32)previous_footer_pt-(*previous_footer_pt)+2*sizeof(int));
		struct BlockElement *nextBlock = (struct BlockElement *)((uint32)va+get_block_size(va));
		LIST_REMOVE(&freeBlocksList,nextBlock);
		set_block_data(previousBlock,(*previous_footer_pt+*header_pt+*next_header_pt),0);
	}	// case previous block is free
	else if(!(*previous_footer_pt & 1))
	{
		struct BlockElement *previousBlock =(struct BlockElement *) ((uint32)previous_footer_pt-(*previous_footer_pt)+2*sizeof(int));
		set_block_data(previousBlock,(*previous_footer_pt+*header_pt),0);
	}
	// case next block is free
	else if(!(*next_header_pt & 1 ))
	{
		struct BlockElement *nextBlock = (struct BlockElement *)((uint32)va+get_block_size(va));
		struct BlockElement *newBlock =(struct BlockElement *)block;
		set_block_data(newBlock,(*header_pt+*next_header_pt),0);
		LIST_INSERT_BEFORE(&freeBlocksList,nextBlock,newBlock);
		LIST_REMOVE(&freeBlocksList,nextBlock);
	}	// case both previous and next allocated
	else
	{
		set_block_data(block,(*header_pt),0);
		bool inserted = 0;
		struct BlockElement *sorted_block;
		LIST_FOREACH(sorted_block,&freeBlocksList)
		{
			if(sorted_block>block)
			{
				inserted = 1;
				LIST_INSERT_BEFORE(&freeBlocksList, sorted_block,block);
				break;
			}
		}
		if(!inserted)
		LIST_INSERT_TAIL(&freeBlocksList,block);
	}
	/*int sum=0;
	LIST_FOREACH(block,&freeBlocksList)
		{
			sum+=get_block_size(block);
		}
	cprintf("b: %d\n",sum);
	cprintf("-----------------------------------------------------------\n");*/

}


//=========================================
// [6] REALLOCATE BLOCK BY FIRST FIT:
//=========================================

void *realloc_block_FF(void *va, uint32 new_size)
{
        //TODO: [PROJECT'24.MS1 - #08] [3] DYNAMIC ALLOCATOR - realloc_block_FF
        //COMMENT THE FOLLOWING LINE BEFORE START CODING
        //panic("realloc_block_FF is not implemented yet");
	// 3 special cases
	if(new_size==0)
	{
		free_block(va);
		return NULL ;
	}
	if(va==NULL)
	{
	return alloc_block_FF(new_size);
	}
	new_size+=2*sizeof(int);
	uint32 old_size = get_block_size(va) ;
	struct BlockElement *the_next_block =(struct BlockElement *)((uint32)va + old_size);
	uint32 *next_header_pt = (uint32 *)((uint32)va+old_size-sizeof(int));
	// case decreasing size
	if(new_size<old_size)
	{
	    // size enough to add new free block
		if(old_size-new_size>=16)
		{
			set_block_data(va,new_size,1);
			struct BlockElement *newBlock =(struct BlockElement *)((uint32)va + new_size);
			// the_next_block may be allocated
			if(!(*next_header_pt & 1 ))
			{
				set_block_data(newBlock,old_size-new_size+get_block_size(the_next_block),0);
				LIST_INSERT_BEFORE(&freeBlocksList,the_next_block,newBlock);
				LIST_REMOVE(&freeBlocksList,the_next_block);
			}
			else
			{
				set_block_data(newBlock,old_size-new_size,0);
				bool inserted = 0;
				struct BlockElement *sorted_block;
				LIST_FOREACH(sorted_block,&freeBlocksList)
				{
					if(sorted_block>newBlock)
					{
						inserted = 1;
						LIST_INSERT_BEFORE(&freeBlocksList, sorted_block,newBlock);
						break;
					}
				}
				if(!inserted)
				LIST_INSERT_TAIL(&freeBlocksList,newBlock);
			}
		}
		// small size (internal fragmentation)
		else
		{
			set_block_data(va,old_size,1);
		}
		return va;
	}// case increasing size
	else if(new_size>old_size)
	{
		uint32 diff = new_size-old_size;
		uint32 next_block_size = get_block_size(the_next_block);
		// sufficient free block in front of the allocated block
		  if (!(*next_header_pt & 1) && (next_block_size >= diff))
		  {
			 struct BlockElement *newBlock =(struct BlockElement *)((uint32)va + new_size);
			 uint32 new_block_size = next_block_size - diff;
			 // size enough to add new free block
			 if(new_block_size>=16)
			 {
				 set_block_data(va,new_size,1);
				 set_block_data(newBlock,new_block_size,0);
				 LIST_INSERT_BEFORE(&freeBlocksList,the_next_block,newBlock);
				 LIST_REMOVE(&freeBlocksList,the_next_block);
			 }
			 else
			 {// small size (internal fragmentation)
				 set_block_data(va,new_size+new_block_size,1);
				 LIST_REMOVE(&freeBlocksList,the_next_block);
			 }
				  return va;
		  }
		  else
		  {   // No sufficient free block in front of the allocated block
			  struct BlockElement *newBlock  = alloc_block_FF(new_size-2*sizeof(int));
			  if (newBlock != NULL)
			  {
	  				//copy the old block data to the new block ?
	  				memcpy(newBlock, va, old_size);
	  				free_block(va);
	  				return newBlock;
			  }// not enough size in all free blocks
	  		  else
	  		  {
	  			  sbrk(0);
	  			  return va;
	  		  }
		  }
	}  // old and new sizes are equal
	else
	{
		return va;
	}
}

/*********************************************************************************************/
/*********************************************************************************************/
/*********************************************************************************************/
//=========================================
// [7] ALLOCATE BLOCK BY WORST FIT:
//=========================================
void *alloc_block_WF(uint32 size)
{
	panic("alloc_block_WF is not implemented yet");
	return NULL;
}

//=========================================
// [8] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
void *alloc_block_NF(uint32 size)
{
	panic("alloc_block_NF is not implemented yet");
	return NULL;
}
