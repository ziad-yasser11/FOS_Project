#include <inc/lib.h>

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//=============================================
// [1] CHANGE THE BREAK LIMIT OF THE USER HEAP:
//=============================================

bool markedVa[(USER_HEAP_MAX-(USER_HEAP_START+DYN_ALLOC_MAX_SIZE+PAGE_SIZE))/PAGE_SIZE];
uint32 spaceInfo[(USER_HEAP_MAX-(USER_HEAP_START+DYN_ALLOC_MAX_SIZE+PAGE_SIZE))/PAGE_SIZE];
uint32 shared_id[100]= {0};
uint32 sharedize[100]= {0};

//bool markedV[3][(USER_HEAP_MAX-(USER_HEAP_START+DYN_ALLOC_MAX_SIZE+PAGE_SIZE))/PAGE_SIZE];
void* sbrk(int increment)
{
	return (void*) sys_sbrk(increment);
}

//=================================
// [2] ALLOCATE SPACE IN USER HEAP:
//=================================
void* malloc(uint32 size)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	if (size == 0) return NULL ;
	if(size <= DYN_ALLOC_MAX_BLOCK_SIZE)
	{
		return alloc_block_FF(size);
	}
	size = ROUNDUP(size,PAGE_SIZE);
	uint32 start_address = (uint32)myEnv->U_heap_limit+PAGE_SIZE;
	uint32 end_address = USER_HEAP_MAX;
	uint32 numOfPages = size / PAGE_SIZE;
	uint32 freePages = 0;
	uint32 start_allocVA=start_address;
	for(uint32 va = start_address;va<end_address;va+=PAGE_SIZE)
	{
	  if(freePages == numOfPages)
	  {
		  break;
	  }
	  uint32 pageNo = (va - start_address) / PAGE_SIZE;
	  if(!markedVa[pageNo])
	  {
		  freePages++;
		  if(freePages == 1)
			  start_allocVA = va;
	  }
	  else
	  {
		  freePages = 0;
	  }
	}
	if(freePages != numOfPages)
		return NULL;
	uint32 pageNo = (start_allocVA - start_address) / PAGE_SIZE;
	spaceInfo[pageNo] = size;
	for(uint32 i =pageNo;i<numOfPages+pageNo;i++)
	{
		markedVa[i] = 1;
	}
	sys_allocate_user_mem(start_allocVA,size);
	return (void *)start_allocVA;
	//==============================================================
	//TODO: [PROJECT'24.MS2 - #12] [3] USER HEAP [USER SIDE] - malloc()
	// Write your code here, remove the panic and write your code
	//panic("malloc() is not implemented yet...!!");
	//Use sys_isUHeapPlacementStrategyFIRSTFIT() and	sys_isUHeapPlacementStrategyBESTFIT()
	//to check the current strategy

}


//=================================
// [3] FREE SPACE FROM USER HEAP:
//=================================
void free(void* virtual_address)
{
    //TODO: [PROJECT'24.MS2 - #14] [3] USER HEAP [USER SIDE] - free()
    // Write your code here, remove the panic and write your code
    //panic("free() is not implemented yet...!!");
	uint32 start_address = (uint32)myEnv->U_heap_limit+PAGE_SIZE;
    if((uint32) virtual_address <(uint32)myEnv->U_heap_start || (uint32) virtual_address > USER_HEAP_MAX )
                panic("invalid address");
    if((uint32) virtual_address < start_address - PAGE_SIZE)
    {
        free_block(virtual_address);
        return;
    }
    virtual_address = ROUNDDOWN(virtual_address,PAGE_SIZE);
    uint32 pageNo = ((uint32)virtual_address - start_address) / PAGE_SIZE;
    if(!markedVa[pageNo])
    	panic("invalid address");
    uint32 size = spaceInfo[pageNo];
    uint32 numOfPages = size/PAGE_SIZE;
    for(uint32 i =pageNo;i<numOfPages+pageNo;i++)
	{
		markedVa[i] = 0;
	}
    sys_free_user_mem((uint32)virtual_address,size);
}

//=================================
// [4] ALLOCATE SHARED VARIABLE:
//=================================
void* smalloc(char* sharedVarName, uint32 size, uint8 isWritable)
{
    //==============================================================
    //DON'T CHANGE THIS CODE========================================

    //==============================================================
    //TODO: [PROJECT'24.MS2 - #18] [4] SHARED MEMORY [USER SIDE] - smalloc()
    // Write your code here, remove the panic and write your code
    //panic("smalloc() is not implemented yet...!!");

     if (size == 0)
     {
            return NULL;
     }

     	if(size >  (USER_HEAP_MAX - ((uint32)myEnv->U_heap_limit+PAGE_SIZE)))
 			return NULL;

        size = ROUNDUP(size, PAGE_SIZE);
        uint32 start_address = (uint32)myEnv->U_heap_limit + PAGE_SIZE;
        uint32 end_address = USER_HEAP_MAX;
        uint32 numOfPages = size / PAGE_SIZE;
        uint32 freePages = 0;
        uint32 alloc_va = start_address ;
        uint32 indexx = 0;
        for(uint32 va = start_address ; va < end_address ; va+=PAGE_SIZE)
            {

            uint32 index = (va - start_address) / PAGE_SIZE;
              if(markedVa[index] == 0)
              {

            	  freePages++;
            	  if(freePages == 1){
            		  alloc_va = va;
            	  }
              }

              else
              {
                  freePages = 0;
              }

              if(freePages==numOfPages){
            	  break;
              }
            }

        if (freePages != numOfPages) {
                return NULL;
            }
        if (alloc_va == 0 || alloc_va + size > USTACKTOP) {
                return NULL;
            }

        uint32 pageNo = (alloc_va - start_address) / PAGE_SIZE;

		for(uint32 i =pageNo;i<numOfPages+pageNo;i++)
		{
			markedVa[i] = 1;
		}
		uint32 perm=0;
		  if (isWritable){
			  perm = PERM_WRITEABLE;
		  }

        int ret = sys_createSharedObject(sharedVarName,size,perm,(void* )alloc_va);
        if(ret<0){

        	for(uint32 zz = alloc_va;zz<alloc_va +(numOfPages*PAGE_SIZE);zz+=PAGE_SIZE ){
				uint32 index = (alloc_va - start_address) / PAGE_SIZE;
				markedVa[index] = 0;

				}
        	 return NULL;
        }

          shared_id[pageNo] = ret;
          sharedize[pageNo] = size;

        return (void*)alloc_va ;
}

//========================================
// [5] SHARE ON ALLOCATED SHARED VARIABLE:
//========================================

void* sget(int32 ownerEnvID, char *sharedVarName) {

	//TODO: [PROJECT'24.MS2 - #20] [4] SHARED MEMORY [USER SIDE] - sget()
		// Write your code here, remove the panic and write your code
		//panic("sget() is not implemented yet...!!");
       uint32 shared_size = sys_getSizeOfSharedObject(ownerEnvID, sharedVarName);
       if(shared_size==E_SHARED_MEM_NOT_EXISTS){
    	   return NULL;
    	}
       shared_size = ROUNDUP(shared_size,PAGE_SIZE);
       	uint32 start_address = (uint32)myEnv->U_heap_limit+PAGE_SIZE;
       	uint32 end_address = USER_HEAP_MAX;
       	uint32 numOfPages = shared_size / PAGE_SIZE;
       	uint32 freePages = 0;
       	uint32 start_allocVA=start_address;
       	for(uint32 va = start_address;va<end_address;va+=PAGE_SIZE)
       	{
       	  if(freePages == numOfPages)
       	  {
       		  break;
       	  }
       	  uint32 pageNo = (va - start_address) / PAGE_SIZE;
       	  if(!markedVa[pageNo])
       	  {
       		  freePages++;
       		  if(freePages == 1)
       			  start_allocVA = va;
       	  }
       	  else
       	  {
       		  freePages = 0;
       	  }
       	}
       	if(freePages != numOfPages){
       		return NULL;
       	}
       	uint32 pageNo = (start_allocVA - start_address) / PAGE_SIZE;
       	for(uint32 i =pageNo;i<numOfPages+pageNo;i++)
       	{
       		markedVa[i] = 1;
       	}
      int shared = sys_getSharedObject(ownerEnvID, sharedVarName, (void*) start_allocVA);
      if(shared==E_SHARED_MEM_NOT_EXISTS){
    	  return NULL;
      }
      else{
    	  shared_id[pageNo] = shared;
    	  sharedize[pageNo] = shared_size;
    	  return (void*)start_allocVA;
      }

}



//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//=================================
// FREE SHARED VARIABLE:
//=================================
//	This function frees the shared variable at the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from main memory then switch back to the user again.
//
//	use sys_freeSharedObject(...); which switches to the kernel mode,
//	calls freeSharedObject(...) in "shared_memory_manager.c", then switch back to the user mode here
//	the freeSharedObject() function is empty, make sure to implement it.

void sfree(void* virtual_address)
{

    if (virtual_address == NULL) {
        return;
    }

    uint32 start_address = USER_HEAP_START + DYN_ALLOC_MAX_SIZE + PAGE_SIZE;
    uint32 va = (uint32)virtual_address;
    if (va < start_address || va >= USER_HEAP_MAX) {
        return;
    }

    uint32 pageNo = (va - start_address) / PAGE_SIZE;
    uint32 shared_id_value = shared_id[pageNo];
    uint32 id = (int32)virtual_address & 0x7FFFFFFF;

    if (shared_id_value < 0) {
        return;
    }

    uint32 shared_size = spaceInfo[pageNo];
    uint32 numOfPages = shared_size / PAGE_SIZE;

    int ret = sys_freeSharedObject(shared_id_value, virtual_address);
    if (ret < 0) {
        return;
    }

    for (uint32 i = pageNo; i < pageNo + numOfPages; i++) {
        markedVa[i] = 0;
    }
    sharedize[pageNo] = 0;
    shared_id[pageNo] = 0;

}

//=================================
// REALLOC USER SPACE:
//=================================
//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_move_user_mem(...)
//		which switches to the kernel mode, calls move_user_mem(...)
//		in "kern/mem/chunk_operations.c", then switch back to the user mode here
//	the move_user_mem() function is empty, make sure to implement it.
void *realloc(void *virtual_address, uint32 new_size)
{
	//[PROJECT]
	// Write your code here, remove the panic and write your code
	panic("realloc() is not implemented yet...!!");
	return NULL;

}


//==================================================================================//
//========================== MODIFICATION FUNCTIONS ================================//
//==================================================================================//

void expand(uint32 newSize)
{
	panic("Not Implemented");

}
void shrink(uint32 newSize)
{
	panic("Not Implemented");

}
void freeHeap(void* virtual_address)
{
	panic("Not Implemented");

}
