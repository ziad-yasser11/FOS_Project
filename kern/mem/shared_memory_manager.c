#include <inc/memlayout.h>
#include "shared_memory_manager.h"

#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/queue.h>
#include <inc/environment_definitions.h>

#include <kern/proc/user_environment.h>
#include <kern/trap/syscall.h>
#include "kheap.h"
#include "memory_manager.h"
//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//
struct Share* get_share(int32 ownerID, char* name);
//struct sleeplock shareLock;

//===========================
// [1] INITIALIZE SHARES:
//===========================
//Initialize the list and the corresponding lock
void sharing_init()
{
#if USE_KHEAP
	LIST_INIT(&AllShares.shares_list) ;
	init_spinlock(&AllShares.shareslock, "shares lock");
#else
	panic("not handled when KERN HEAP is disabled");
#endif
}

//==============================
// [2] Get Size of Share Object:
//==============================
int getSizeOfSharedObject(int32 ownerID, char* shareName)
{
	//[PROJECT'24.MS2] DONE
	// This function should return the size of the given shared object
	// RETURN:
	//	a) If found, return size of shared object
	//	b) Else, return E_SHARED_MEM_NOT_EXISTS
	//
	struct Share* ptr_share = get_share(ownerID, shareName);
	if (ptr_share == NULL)
		return E_SHARED_MEM_NOT_EXISTS;
	else
		return ptr_share->size;

	return 0;
}

//===========================================================


//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
//===========================
// [1] Create frames_storage:
//===========================
// Create the frames_storage and initialize it by 0
inline struct FrameInfo** create_frames_storage(int numOfFrames)
{
	//TODO: [PROJECT'24.MS2 - #16] [4] SHARED MEMORY - create_frames_storage()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("create_frames_storage is not implemented yet");
	//Your Code is Here...

	if(numOfFrames == 0){
		return NULL;

	}

	struct FrameInfo** framesStorage = (struct FrameInfo**)kmalloc(numOfFrames * sizeof(struct FrameInfo*));
	for (int i = 0; i < numOfFrames; i++) {
		framesStorage[i] = 0;
	    }
	if(framesStorage==NULL){
		return NULL; //If failed: NULL
	}
	else {

	return framesStorage; // If succeed: pointer to the created array

	}

}

//=====================================
// [2] Alloc & Initialize Share Object:
//=====================================
//Allocates a new shared object and initialize its member
//It dynamically creates the "framesStorage"
//Return: allocatedObject (pointer to struct Share) passed by reference
struct Share* create_share(int32 ownerID, char* shareName, uint32 size, uint8 isWritable)
{
    //TODO: [PROJECT'24.MS2 - #16] [4] SHARED MEMORY - create_share()
    //COMMENT THE FOLLOWING LINE BEFORE START CODING
    //panic("create_share is not implemented yet");
    //Your Code is Here...
    struct Share* new_Share = NULL;
    new_Share = (struct Share*)kmalloc(sizeof(struct Share));
     if (new_Share == NULL){
          return NULL;
       }
    new_Share->references = 1;
    new_Share->isWritable =  isWritable;
    strcpy(new_Share->name,shareName);
    new_Share->ownerID = ownerID;
    new_Share->size = size;
    new_Share->ID = (int32)new_Share & 0x7FFFFFFF;
    size=ROUNDUP(size,PAGE_SIZE);
    uint32 number_of_frames = size / PAGE_SIZE;
    new_Share->framesStorage = create_frames_storage(number_of_frames);
    if(new_Share->framesStorage==NULL){
    	kfree(new_Share);
        return NULL;
    }
    return new_Share ;
}
//=============================
// [3] Search for Share Object:
//=============================
//Search for the given shared object in the "shares_list"
//Return:
//	a) if found: ptr to Share object
//	b) else: NULL
struct Share* get_share(int32 ownerID, char* name)
{
	//TODO: [PROJECT'24.MS2 - #17] [4] SHARED MEMORY - get_share()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("get_share is not implemented yet");
	//Your Code is Here...
		if (name == NULL) {
			return NULL;
		}
	    acquire_spinlock(&AllShares.shareslock);
	    struct Share* current_share;
	    LIST_FOREACH(current_share, &AllShares.shares_list) {
	        if (current_share->ownerID == ownerID && strcmp(current_share->name, name) == 0) {
	            release_spinlock(&AllShares.shareslock);
	            return current_share;
	        }
	    }
	    release_spinlock(&AllShares.shareslock);
	    return NULL;
}


//=========================
// [4] Create Share Object:
//=========================
int createSharedObject(int32 ownerID, char* shareName, uint32 size, uint8 isWritable, void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #19] [4] SHARED MEMORY [KERNEL SIDE] - createSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("createSharedObject is not implemented yet");
	//Your Code is Here...

	struct Env* myenv = get_cpu_proc(); //The calling environment
	if (get_share(ownerID, shareName) != NULL) {
		        return E_SHARED_MEM_EXISTS;
		    }
    acquire_spinlock(&AllShares.shareslock);
	struct Share* newSharedObject = create_share(ownerID,shareName,size,isWritable);
	if(newSharedObject == NULL){
	    release_spinlock(&AllShares.shareslock);
		return E_NO_SHARE ;
	}
    if (newSharedObject->framesStorage == NULL) {
	    release_spinlock(&AllShares.shareslock);
        return E_NO_SHARE;
    }
	uint32 num_frames = ROUNDUP(size,PAGE_SIZE) / PAGE_SIZE ;
	uint32 start_address = (uint32)virtual_address;
	for(uint32 i = 0 ; i < num_frames ; i++){

		struct FrameInfo * ptr_frame_info;
		int allocateRet = allocate_frame(&ptr_frame_info);
		        if (allocateRet == E_NO_MEM) {
		        	for (uint32 j = 0; j < i; j++){
		        		free_frame(newSharedObject->framesStorage[j]);

		        	}
		    	    release_spinlock(&AllShares.shareslock);
		            return E_NO_MEM;
		        }

		        int map_result = map_frame(myenv->env_page_directory, ptr_frame_info, start_address,PERM_WRITEABLE|PERM_PRESENT|PERM_USER);
		             if (map_result != 0) {
		            	  for (uint32 j = 0; j < i; j++) {
		            		  free_frame(newSharedObject->framesStorage[j]);
		            	  }
		          	    release_spinlock(&AllShares.shareslock);
		                 return map_result;
		             }
		             newSharedObject->framesStorage[i]=ptr_frame_info;
		             start_address += PAGE_SIZE;
	}

    LIST_INSERT_HEAD(&AllShares.shares_list, newSharedObject);
    release_spinlock(&AllShares.shareslock);
    return newSharedObject->ID;
}

//======================
// [5] Get Share Object:
//======================
int getSharedObject(int32 ownerID, char* shareName, void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #21] [4] SHARED MEMORY [KERNEL SIDE] - getSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("getSharedObject is not implemented yet");
	//Your Code is Here...

	struct Env* myenv = get_cpu_proc(); //The calling environment
    struct Share* sharedObject = get_share(ownerID, shareName);
    if (sharedObject == NULL) {
          return E_SHARED_MEM_NOT_EXISTS;
      }

    int size = sharedObject->size;
    uint32 num_frames = ROUNDUP(size,PAGE_SIZE) / PAGE_SIZE ;
    uint32 start_address = (uint32)virtual_address;
    for (uint32 i = 0; i < num_frames; i++) {

	   struct FrameInfo* frame = sharedObject->framesStorage[i];
	   if (frame == 0) {
		 sharedObject->references--;
		 return E_SHARED_MEM_NOT_EXISTS;
	         }
		uint32 perm ;
		if (sharedObject->isWritable) {
		perm = PERM_WRITEABLE | PERM_USER|PERM_PRESENT;
		}
		else{
			perm=PERM_USER |PERM_PRESENT;
		}
	   int map_result = map_frame(myenv->env_page_directory, frame, start_address,perm);
			  if (map_result != 0) {

				 sharedObject->references--;
				 return map_result;
			  }
			  start_address += PAGE_SIZE;
		  }

       sharedObject->references++;
       return sharedObject->ID ;
}

//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//==========================
// [B1] Delete Share Object:
//==========================
//delete the given shared object from the "shares_list"
//it should free its framesStorage and the share object itself
void free_share(struct Share* ptrShare)
{
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [KERNEL SIDE] - free_share()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("free_share is not implemented yet");
	//Your Code is Here...

	if(ptrShare == NULL){
		return ;
	}

	acquire_spinlock(&AllShares.shareslock);
	struct Share* share;
	bool found = 0;
	LIST_FOREACH(share, &AllShares.shares_list)
	{
	    if (share == ptrShare)
	    {
	        found = 1;
	        break;
	    }
	}

	if (found)
	{

	    LIST_REMOVE(&(AllShares.shares_list), ptrShare);
	}
	release_spinlock(&AllShares.shareslock);

	if (!found)
	{
	    return;
	}
	  int numOfPages = ROUNDUP(ptrShare->size, PAGE_SIZE) / PAGE_SIZE;
	    for (int i = 0; i < numOfPages; i++) {
	        if (ptrShare->framesStorage[i] != NULL) {
	        	ptrShare->framesStorage[i] = NULL;

	        }
	    }

	 kfree(ptrShare->framesStorage);
	 kfree(ptrShare);

}
//========================
// [B2] Free Share Object:
struct Share* get_share_by_id(int32 sharedObjectID)
{
    struct Share* share;
    LIST_FOREACH(share, &AllShares.shares_list) {
        if (share->ID == sharedObjectID) {
            return share;
        }
    }
    return NULL;
}

//========================
int freeSharedObject(int32 sharedObjectID, void *startVA)
{


	 acquire_spinlock(&AllShares.shareslock);
	    struct Env* myenv = get_cpu_proc();

	    struct Share *shared_object ;
	    LIST_FOREACH(shared_object, &AllShares.shares_list) //Get the shared object from the "shares_list" -> done

	      {
	          if (shared_object->ID == sharedObjectID)
	          {

	              break;
	          }
	      }
    if (shared_object == NULL)
    {
        release_spinlock(&AllShares.shareslock);
        return E_NO_SHARE; // Shared object not found
    }

    release_spinlock(&AllShares.shareslock);
    int numOfPages = ROUNDUP(shared_object->size, PAGE_SIZE) / PAGE_SIZE;
    uint32 va = (uint32)startVA;
    uint32* ptr_page_table = NULL;
    for (int i = 0; i < numOfPages; i++) {
        uint32 pageVA = va + i * PAGE_SIZE;
        struct FrameInfo *frame = get_frame_info(myenv->env_page_directory, pageVA, &ptr_page_table);

        if (frame != NULL) {
            unmap_frame(myenv->env_page_directory, pageVA);
           // free_frame(frame);

        }
        if(ptr_page_table!=NULL){

                bool is_empty = 1;
        		for(int i=0;i<1024;i++){

        			 if (ptr_page_table[i] & PERM_PRESENT){
        				 is_empty=0;
        				 break;
        			 }

        		}
        		 if (is_empty) {

        			 kfree(ptr_page_table);
        			 pd_clear_page_dir_entry(myenv->env_page_directory,pageVA);
        		 }
            }

    }

    shared_object->references--;
    if (shared_object->references == 0)
    {
        free_share(shared_object);

    }

    tlbflush();
    return 0;
}
