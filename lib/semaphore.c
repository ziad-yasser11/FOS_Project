// User-level Semaphore

#include "inc/lib.h"

struct semaphore create_semaphore(char *semaphoreName, uint32 value)
{
	//TODO: [PROJECT'24.MS3 - #02] [2] USER-LEVEL SEMAPHORE - create_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("create_semaphore is not implemented yet");
	//Your Code is Here...
	int addr = (int)smalloc(semaphoreName, sizeof(struct __semdata), 1);
	struct semaphore sem;
	sem.semdata = (struct __semdata*)addr;
	sem.semdata->count = (int)value;
	strcpy(sem.semdata->name,semaphoreName);
	sem.semdata->lock = 0;
	sys_init_queue(&sem.semdata->queue);
	return sem;
}

struct semaphore get_semaphore(int32 ownerEnvID, char* semaphoreName)
{
	//TODO: [PROJECT'24.MS3 - #03] [2] USER-LEVEL SEMAPHORE - get_semaphore
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("get_semaphore is not implemented yet");
	//Your Code is Here...
	struct semaphore get_semaphore;
	get_semaphore.semdata = sget(ownerEnvID,semaphoreName);
	return get_semaphore;
}

void wait_semaphore(struct semaphore sem)
{
    //TODO: [PROJECT'24.MS3 - #04] [2] USER-LEVEL SEMAPHORE - wait_semaphore
    //COMMENT THE FOLLOWING LINE BEFORE START CODING
    //panic("wait_semaphore is not implemented yet");
    //Your Code is Here...
    //struct Env *current_process = sys_get_cpu_proc();//current proc (get_cpu_proc need syscall)
    int keyw = 1;
    while(xchg(&(sem.semdata->lock),keyw) !=0);
    sem.semdata->count--;
      if(sem.semdata->count < 0){
          sys_enqueue(&sem); //enqueue in semaphore queue
       }
        sem.semdata->lock = 0;

}

void signal_semaphore(struct semaphore sem)
{
    //TODO: [PROJECT'24.MS3 - #05] [2] USER-LEVEL SEMAPHORE - signal_semaphore
    //COMMENT THE FOLLOWING LINE BEFORE START CODING
    //panic("signal_semaphore is not implemented yet");
    //Your Code is Here...

    int keys = 1;
        while(xchg(&(sem.semdata->lock),keys) !=0);

        sem.semdata->count++;
        if(sem.semdata->count<=0){
          sys_dequeue(&(sem.semdata->queue));//dequeue from semaphore queue
        }
        sem.semdata->lock = 0;

}

int semaphore_count(struct semaphore sem)
{
	return sem.semdata->count;
}
