/*
 * channel.c
 *
 *  Created on: Sep 22, 2024
 *      Author: HP
 */
#include "channel.h"
#include <kern/proc/user_environment.h>
#include <kern/cpu/sched.h>
#include <inc/string.h>
#include <inc/disk.h>

//===============================
// 1) INITIALIZE THE CHANNEL:
//===============================
// initialize its lock & queue
void init_channel(struct Channel *chan, char *name)
{
	strcpy(chan->name, name);
	init_queue(&(chan->queue));
}

//===============================
// 2) SLEEP ON A GIVEN CHANNEL:
//===============================
// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
// Ref: xv6-x86 OS code
void sleep(struct Channel *chan, struct spinlock *lk) {
    acquire_spinlock(&ProcessQueues.qlock);
    release_spinlock(lk);

    struct Env *current_process = get_cpu_proc();
    current_process->env_status = ENV_BLOCKED;
    current_process->channel=chan;
    enqueue(&chan->queue, current_process);

    sched();

    release_spinlock(&ProcessQueues.qlock);
    acquire_spinlock(lk);

}

//==================================================
// 3) WAKEUP ONE BLOCKED PROCESS ON A GIVEN CHANNEL:
//==================================================
// Wake up ONE process sleeping on chan.
// The qlock must be held.
// Ref: xv6-x86 OS code
// chan MUST be of type "struct Env_Queue" to hold the blocked processes
void wakeup_one(struct Channel *chan)
{
	acquire_spinlock(&ProcessQueues.qlock);
	struct Env*process ;
	process = dequeue(&chan->queue);
	if(process!=NULL && process->channel==chan){
	process->env_status=ENV_READY;

	sched_insert_ready0(process);
	}

 release_spinlock(&ProcessQueues.qlock);
}

//====================================================
// 4) WAKEUP ALL BLOCKED PROCESSES ON A GIVEN CHANNEL:
//====================================================
// Wake up all processes sleeping on chan.
// The queues lock must be held.
// Ref: xv6-x86 OS code
// chan MUST be of type "struct Env_Queue" to hold the blocked processes

void wakeup_all(struct Channel *chan)
{
    acquire_spinlock(&ProcessQueues.qlock);
    struct Env *p = dequeue(&chan->queue);

    while(p!=NULL && p->channel==chan){

		p->env_status=ENV_READY;
	    sched_insert_ready0(p);
    	p = dequeue(&chan->queue);

	  }

    release_spinlock(&ProcessQueues.qlock);
}
