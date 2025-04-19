#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"

uint32 vaAddr[1024*1024] ;


//Initialize the dynamic allocator of kernel heap with the given start address, size & limit
//All pages in the given range should be allocated
//Remember: call the initialize_dynamic_allocator(..) to complete the initialization
//Return:
//	On success: 0
//	Otherwise (if no memory OR initial size exceed the given limit): PANIC
int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit)
{
	//TODO: [PROJECT'24.MS2 - #01] [1] KERNEL HEAP - initialize_kheap_dynamic_allocator
	// Write your code here, remove the panic and write your code
	//panic("initialize_kheap_dynamic_allocator() is not implemented yet...!!");
	initialize_paging();
	k_heap_start =(uint32 *)(daStart);
	k_heap_limit =(uint32 *)(daLimit);
	k_heap_break = (uint32 *)(daStart+initSizeToAllocate);
	if(daStart+initSizeToAllocate>daLimit )
	{
		panic("Size exceeds the limit");
	}
	uint32 maped_area=0;
	while(initSizeToAllocate>maped_area)
	{
		uint32 new_frame = daStart + maped_area;
		struct FrameInfo *ptr_frame_info;
		int alloc_f = allocate_frame(&ptr_frame_info);
		if(alloc_f == E_NO_MEM){
			panic("No enough memory");
		}
		alloc_f = map_frame(ptr_page_directory,ptr_frame_info,new_frame,PERM_WRITEABLE|PERM_PRESENT);
		uint32 frameNo = to_frame_number(ptr_frame_info);
		vaAddr[frameNo] = new_frame ;
		if(alloc_f == E_NO_MEM)
		{
			panic("No enough memory");
		}
		maped_area+=PAGE_SIZE;
	}
        initialize_dynamic_allocator(daStart,initSizeToAllocate);
        init_sleeplock(&kernLock,"Kernel Sleep Lock");
        init_spinlock(&listFLock,"Fault Spin lock");
        return 0;
}
void* sbrk(int numOfPages)
{
	/* numOfPages > 0: move the segment break of the kernel to increase the size of its heap by the given numOfPages,
	 * 				you should allocate pages and map them into the kernel virtual address space,
	 * 				and returns the address of the previous break (i.e. the beginning of newly mapped memory).
	 * numOfPages = 0: just return the current position of the segment break
	 *
	 * NOTES:
	 * 	1) Allocating additional pages for a kernel dynamic allocator will fail if the free frames are exhausted
	 * 		or the break exceed the limit of the dynamic allocator. If sbrk fails, return -1
	 */

	//MS2: COMMENT THIS LINE BEFORE START CODING==========
	//return (void*)-1 ;
	//====================================================
	//TODO: [PROJECT'24.MS2 - #02] [1] KERNEL HEAP - sbrk
	// Write your code here, remove the panic and write your code
	//panic("sbrk() is not implemented yet...!!");

	uint32 sizeOfPages = numOfPages*PAGE_SIZE;
	if(numOfPages > 0)
	{
		// within the HARDLIMIT
		if(((uint32)(k_heap_break) + sizeOfPages ) <= (uint32) k_heap_limit)
		{
			uint32 ret = (uint32)(k_heap_break);
			k_heap_break = (uint32 *)((uint32)(k_heap_break) + sizeOfPages);
			// allocate pages and map them into the kernel virtual address space
			uint32 va = ret;
			for(int i=0;i<numOfPages;i++)
			{
				struct FrameInfo * ptr_frame_info;
				int allocateRet = allocate_frame(&ptr_frame_info);
				// no memory
				if(allocateRet == E_NO_MEM)
				{
					return (void*)-1 ;
				}
				int mapRet = map_frame(ptr_page_directory,ptr_frame_info,va,PERM_WRITEABLE|PERM_PRESENT);
	        	uint32 frameNo = to_frame_number(ptr_frame_info);
	        	vaAddr[frameNo] = va ;
				if(mapRet == E_NO_MEM)
				{
					return (void*)-1 ;
				}
				va += PAGE_SIZE ;
			}
    		uint32 * EndBlock = (uint32 *)((uint32)k_heap_break- sizeof(int));
			*EndBlock = 0;
			*EndBlock |= 1;
			return (uint32 *)ret;
		}// break exceed the hard limit
		else
		{
			return (void*)-1 ;
		}
	}
	else
	{
		return (void*)k_heap_break;
	}

}

//TODO: [PROJECT'24.MS2 - BONUS#2] [1] KERNEL HEAP - Fast Page Allocator

void* kmalloc(unsigned int size)
{
	//cprintf("pages %d\n",(KERNEL_HEAP_MAX-(KERNEL_HEAP_START+DYN_ALLOC_MAX_SIZE+PAGE_SIZE))/PAGE_SIZE);
	//[PROJECT'24.MS2] Implement this function
	// Write your code here, remove the panic and write your code
	//kpanic_into_prompt("kmalloc() is not implemented yet...!!");

	// use "isKHeapPlacementStrategyFIRSTFIT() ..." functions to check the current strategy
	void* ret = NULL;
	if (size == 0 )
	{
		return NULL;
	}
	if (size <= DYN_ALLOC_MAX_BLOCK_SIZE)
	{ // 2kb
		if(isKHeapPlacementStrategyBESTFIT())
		{
		   ret = alloc_block_BF(size);
		}
		else if(isKHeapPlacementStrategyFIRSTFIT())
		{
			ret = alloc_block_FF(size);
		}
		return ret;
	}
	else
	{
		size = ROUNDUP(size, PAGE_SIZE);
		if(size >  (KERNEL_HEAP_MAX - ((uint32)k_heap_limit+PAGE_SIZE)))
		{
			release_sleeplock(&kernLock);
			return NULL;
		}
		uint32 start_address =(uint32) k_heap_limit+PAGE_SIZE;
		uint32 end_address = KERNEL_HEAP_MAX;
		uint32 numOfPages = size / PAGE_SIZE;
		uint32 freePages = 0;
		uint32 start_allocVA,ret_add;
		struct FreePages *free_pages;

		acquire_sleeplock(&kernLock);

		for(uint32 va = start_address;va<end_address;)
		{
		  uint32 * ptr_page_table;
		  struct FrameInfo *frameInfo;
		  if(freePages == numOfPages)
		  {
			  break;
		  }
		  frameInfo = get_frame_info(ptr_page_directory,va,&ptr_page_table);
		  if(frameInfo == NULL)
		  {
			  freePages++;
			  if(freePages == 1)
				  start_allocVA = va;
			  va+=PAGE_SIZE;
		  }
		  else
		  {
			  va += frameInfo->size;
			  freePages = 0;
		  }
		}
		ret_add = start_allocVA;
		acquire_spinlock(&MemFrameLists.mfllock);
		int listSize = LIST_SIZE(&MemFrameLists.free_frame_list);
		release_spinlock(&MemFrameLists.mfllock);
		if(freePages != numOfPages || listSize< numOfPages)
		{
			release_sleeplock(&kernLock);
			return NULL;
		}
		for(int i=0;i<numOfPages;i++)
		{
		struct FrameInfo * ptr_frame_info ;
		int allocateRet = allocate_frame(&ptr_frame_info);
		// no memory
		if(i == 0)
		{
			ptr_frame_info->size = size ;
		}
		if(allocateRet == E_NO_MEM)
		{
			release_sleeplock(&kernLock);
			return NULL ;
		}
		int mapRet = map_frame(ptr_page_directory,ptr_frame_info,start_allocVA,PERM_WRITEABLE|PERM_PRESENT);
		uint32 frameNo = to_frame_number(ptr_frame_info);
		vaAddr[frameNo] = start_allocVA ;
		if(mapRet == E_NO_MEM)
		{
			release_sleeplock(&kernLock);
			return NULL ;
		}
		start_allocVA += PAGE_SIZE ;
		}
		release_sleeplock(&kernLock);
		return (uint32*)ret_add;
      }
}


void kfree(void* virtual_address)
{
    //TODO: [PROJECT'24.MS2 - #04] [1] KERNEL HEAP - kfree
    // Write your code here, remove the panic and write your code
    //panic("kfree() is not implemented yet...!!");
	uint32 size=0;
	if((uint32) virtual_address <(uint32)k_heap_start || (uint32) virtual_address > KERNEL_HEAP_MAX )
	        panic("invalid address");
	if((uint32) virtual_address>=(uint32)k_heap_start && (uint32) virtual_address< (uint32)k_heap_limit)
	{
		free_block(virtual_address);
		return;
	}
    virtual_address = ROUNDDOWN(virtual_address,PAGE_SIZE);
    uint32 *ptr_page_table ;
    struct FrameInfo *ptr_frame_info ;
    ptr_frame_info = get_frame_info(ptr_page_directory,(uint32)virtual_address,&ptr_page_table);
    size = ptr_frame_info->size;
	acquire_sleeplock(&kernLock);

	 for (uint32 i = (uint32)virtual_address ; i < (uint32)virtual_address+size; i+=PAGE_SIZE)
	 {
		 struct FrameInfo* frame_info = get_frame_info(ptr_page_directory, i, &ptr_page_table);
		 if (frame_info != NULL)
		 {
			 uint32 frameNo = to_frame_number(frame_info);
			 vaAddr[frameNo] = 0;
		 }
		 unmap_frame(ptr_page_directory,i);
	 }
	 release_sleeplock(&kernLock);
    //you need to get the size of the given allocation using its address
    //refer to the project presentation and documentation for details

}
unsigned int kheap_physical_address(unsigned int virtual_address)
{
    //TODO: [PROJECT'24.MS2 - #05] [1] KERNEL HEAP - kheap_physical_address
    // Write your code here, remove the panic and write your code
    //panic("kheap_physical_address() is not implemented yet...!!");
	 acquire_sleeplock(&kernLock);
	 if (virtual_address < (unsigned int)k_heap_start && virtual_address >= (unsigned int)k_heap_limit)
	 {
		 release_sleeplock(&kernLock);
	      return 0;
	 }
	     uint32* page_table = NULL;
         get_page_table(ptr_page_directory, virtual_address, &page_table);

		 uint32 page_table_index = PTX(virtual_address);

		 unsigned int offset = virtual_address % PAGE_SIZE;
	     uint32 page_table_entry = page_table[page_table_index];
	     int fram_num = page_table_entry >> 12 ;
	         if ((page_table_entry & PERM_PRESENT) == 0)
	         {
	        	 release_sleeplock(&kernLock);
	             return 0;
	         }
	  struct FrameInfo* ptr_frame_info = get_frame_info(ptr_page_directory,virtual_address, &page_table);
	  release_sleeplock(&kernLock);
	  return fram_num * PAGE_SIZE+ offset ;

    //return the physical address corresponding to given virtual_address
    //refer to the project presentation and documentation for details

    //EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================
}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
    //TODO: [PROJECT'24.MS2 - #06] [1] KERNEL HEAP - kheap_virtual_address
    // Write your code here, remove the panic and write your code
    //panic("kheap_virtual_address() is not implemented yet...!!");
	acquire_sleeplock(&kernLock);
    uint32 offset = physical_address % PAGE_SIZE;
    struct FrameInfo* ptr_frame_info = to_frame_info(physical_address);

    if (ptr_frame_info == NULL){
    	release_sleeplock(&kernLock);
        return 0;
    }
     uint32 frameNo = to_frame_number(ptr_frame_info);
     if (vaAddr[frameNo] == 0)
        {
    	    release_sleeplock(&kernLock);
            return 0;
        }
     release_sleeplock(&kernLock);
     return vaAddr[frameNo]+offset;

    //return the virtual address corresponding to given physical_address
    //refer to the project presentation and documentation for details

    //EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================
}

//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, if moved to another loc: the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().


void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT'24.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc
	// Write your code here, remove the panic and write your code
	/*if(virtual_address == NULL)
	{
		return kmalloc(new_size);
	}
	if(new_size == 0)
	{
	    kfree(virtual_address);
	    return NULL;
	}
	uint32 size=0;
	if((uint32) virtual_address <(uint32)k_heap_start || (uint32) virtual_address > KERNEL_HEAP_MAX )
	        panic("invalid address");
	// if va of block
	if((uint32) virtual_address>=(uint32)k_heap_start && (uint32) virtual_address< (uint32)k_heap_limit)
	{
		if(new_size <= DYN_ALLOC_MAX_BLOCK_SIZE)
			return realloc_block_FF(virtual_address,new_size);
		else
		{
			uint32 * newAddr = kmalloc(new_size);
			memcpy(newAddr, virtual_address, get_block_size(virtual_address));
	        free_block(virtual_address);
	        return newAddr;
		}
	}
	virtual_address = ROUNDDOWN(virtual_address,PAGE_SIZE);
    uint32 *ptr_page_table ;
    struct FrameInfo *ptr_frame_info ;
    ptr_frame_info = get_frame_info(ptr_page_directory,(uint32)virtual_address,&ptr_page_table);
    size = ptr_frame_info->size;
    new_size = ROUNDUP(new_size,PAGE_SIZE);
    if(new_size == size)
    	return virtual_address;
    else if(new_size > size)
    {
    	uint32 diff = new_size - size;
    	// if next is free
    	uint32 startAddr = ((uint32)virtual_address+size);
    	uint32 noOfPages = diff/PAGE_SIZE;
    	uint32 next_freePages = 0;
    	uint32 prev_freePages = 0;
    	bool found = 0;
		for(uint32 va = startAddr;va<KERNEL_HEAP_MAX;va+=PAGE_SIZE)
		{
		  uint32 * ptr_page_table;
		  if(get_frame_info(ptr_page_directory,va,&ptr_page_table) == NULL)
		  {
			  next_freePages++;
		  }
		  else
			  break;
		}
		for(uint32 va = (uint32)virtual_address-PAGE_SIZE;va>=(uint32)k_heap_limit+PAGE_SIZE;va-=PAGE_SIZE)
		{
		  uint32 * ptr_page_table;
		  if(get_frame_info(ptr_page_directory,va,&ptr_page_table) == NULL)
		  {
			  prev_freePages++;
		  }
		  else
			  break;
		}
		if(next_freePages == noOfPages)
		{
			found = 1;
		}
		if(prev_freePages == noOfPages)
		{
			startAddr = (uint32)virtual_address - diff;
			found = 1;
		}
		else if(prev_freePages + next_freePages>= noOfPages)
		{
			startAddr = (uint32)virtual_address - (prev_freePages*PAGE_SIZE);
			found = 1;
		}
		if(found)
		{
			for(int i=0;i<noOfPages;i++)
			{
				if(startAddr == (uint32)virtual_address)
				{
					startAddr = (uint32)virtual_address + size;
				}
				struct FrameInfo * ptr_frame_info1 ;
				int allocateRet = allocate_frame(&ptr_frame_info1);
				// no memory
				if(allocateRet == E_NO_MEM)
				{
					return NULL ;
				}
				int mapRet = map_frame(ptr_page_directory,ptr_frame_info1,startAddr,PERM_WRITEABLE|PERM_PRESENT);
				uint32 frameNo = to_frame_number(ptr_frame_info1);
				vaAddr[frameNo] = startAddr ;
				if(mapRet == E_NO_MEM)
				{
					return NULL ;
				}
				startAddr += PAGE_SIZE ;
			}
			if(next_freePages == noOfPages)
			{
				ptr_frame_info->size = new_size;
				return virtual_address;
			}
			else if(prev_freePages == noOfPages)
			{
				startAddr = (uint32)virtual_address - diff;
				ptr_frame_info->size = 0;
				ptr_frame_info = get_frame_info(ptr_page_directory,(uint32)startAddr,&ptr_page_table);
			    ptr_frame_info->size = new_size;
			    memcpy((void *)startAddr,virtual_address,new_size);
			    return (void *)startAddr;
			}
			else
			{
				startAddr = (uint32)virtual_address - (prev_freePages*PAGE_SIZE);
				ptr_frame_info->size = 0;
				ptr_frame_info = get_frame_info(ptr_page_directory,(uint32)startAddr,&ptr_page_table);
			    ptr_frame_info->size = new_size;
			    memcpy((void *)startAddr,virtual_address,new_size);
			    return (void *)startAddr;
			}
		}
		else
		{
			// allocate new address
			  uint32 * newAddr = kmalloc(new_size);
			  if(newAddr != NULL)
			  {
				  memcpy(newAddr, virtual_address, size);
				  kfree(virtual_address);
			  }
			  return newAddr;
		}
    }
    else
    {
    	if(new_size <= DYN_ALLOC_MAX_BLOCK_SIZE)
    	{
    		uint32* newAddr = alloc_block_FF(new_size);
    		if(newAddr != NULL)
    		{
        		memcpy(newAddr, virtual_address, new_size);
        		kfree(virtual_address);
    		}
    	    return newAddr;
    	}
    	else
    	{
    		uint32 startAddr = (uint32)virtual_address+new_size;
    		uint32 endAddr = (uint32)virtual_address+size;
    		for(uint32 va = startAddr;va<endAddr;va+=PAGE_SIZE)
    		{
    			uint32* ptr_page_table1;
    			struct FrameInfo* frame_info = get_frame_info(ptr_page_directory, va, &ptr_page_table1);
				if (frame_info != NULL)
				{
					uint32 frameNo = to_frame_number(frame_info);
					vaAddr[frameNo] = 0;
				}
    			unmap_frame(ptr_page_directory,va);
    		}
    		ptr_frame_info->size = new_size;
    		return virtual_address;
    	}
    }*/
   panic("krealloc() is not implemented yet...!!");
}
