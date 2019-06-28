/*
     File        : blocking_disk.c

     Author      : 
     Modified    : 

     Description : 

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "blocking_disk.H"
#include "scheduler.H"
#include "simple_disk.H"
#include "thread.H"

extern Scheduler * SYSTEM_SCHEDULER;
/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

BlockingDisk::BlockingDisk(DISK_ID _disk_id, unsigned int _size) 
  : SimpleDisk(_disk_id, _size) {
#ifdef INTERRUPTS_ENABLED
    head_queue = NULL;
    tail_queue = NULL;
#endif
      
}

/*--------------------------------------------------------------------------*/
/* BLOCKING_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

void BlockingDisk::wait_until_ready() {
	while (!is_ready()) 
    {
#ifdef INTERRUPTS_ENABLED
        push (Thread::CurrentThread());
#else
		SYSTEM_SCHEDULER->resume(Thread::CurrentThread());
#endif
		SYSTEM_SCHEDULER->yield();
	}
}

bool BlockingDisk::blocking_is_ready() {
    SimpleDisk::is_ready();
}

#ifdef INTERRUPTS_ENABLED
Thread * BlockingDisk::pop() 
{

    if (head_queue != NULL)
    {
        ready_queue *temp_queue_entry = head_queue;
        Thread * next_thread_id = temp_queue_entry->thread_id;
        // Update the head
        head_queue = head_queue->next;
        if (head_queue == NULL)
            tail_queue = NULL;

        // Delete the queue entry
        delete temp_queue_entry;
        return next_thread_id;
    }
}

void BlockingDisk::push(Thread * _thread)
{
    if (head_queue == NULL || tail_queue == NULL)
    {
        // First entry in queue
        ready_queue *new_queue_entry = new ready_queue;
        new_queue_entry->thread_id = _thread;
        head_queue = tail_queue = new_queue_entry;
        new_queue_entry->next = NULL;
    }
    else
    {
        // Queue already present. Add entry
        ready_queue *new_queue_entry = new ready_queue;
        new_queue_entry->thread_id = _thread;
        tail_queue->next = new_queue_entry;
        tail_queue = new_queue_entry;
        new_queue_entry->next == NULL;
    }
}

void BlockingDisk::handle_interrupt(REGS *_r) {
    Thread * next_thread_id = pop();
    SYSTEM_SCHEDULER->resume(next_thread_id->CurrentThread());

}
#endif
