/*
 File: scheduler.C
 
 Author:
 Date  :
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "simple_timer.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

Scheduler::Scheduler()
{
    //Initialize the queue to NULL
    head_queue = NULL;
    tail_queue = NULL;
    Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() 
{
    // Disable the interrupt if enabled
    if (Machine::interrupts_enabled())
        Machine::disable_interrupts();

    if (head_queue != NULL)
    {
        ready_queue *temp_queue_entry = head_queue;
        Thread * next_thread_id = head_queue->thread_id;
        // Update the head
        head_queue = head_queue->next;
        if (head_queue == NULL)
            tail_queue = NULL;

        // Delete the queue entry
        delete temp_queue_entry;
        // Process next thread
        Thread::dispatch_to(next_thread_id);
    }
    // Enable the interrupt if disabled
    if (!Machine::interrupts_enabled())
        Machine::enable_interrupts();
}

void Scheduler::resume(Thread * _thread) 
{
    // Disable the interrupt if enabled
    if (Machine::interrupts_enabled())
        Machine::disable_interrupts();
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
    // Enable the interrupt if disabled
    if (!Machine::interrupts_enabled())
        Machine::enable_interrupts();
}

void Scheduler::add(Thread * _thread)
{
    resume (_thread);
}

void Scheduler::terminate(Thread * _thread) 
{
    delete _thread;
    yield ();
}


RoundRobinScheduler::RoundRobinScheduler(unsigned EOQ) 
{
    //Initialize the queue to NULL
    head_queue = NULL;
    tail_queue = NULL;
    // Set the timer and register to interrupt handler
    SimpleTimer * timer = new SimpleTimer(1000/EOQ);  
    InterruptHandler::register_handler(0,timer);
    Console::puts("Constructed Round Robin Scheduler.\n");
}

void RoundRobinScheduler::yield() 
{
    // Disable the interrupt if enabled
    if (Machine::interrupts_enabled())
        Machine::disable_interrupts();

    if (head_queue != NULL) 
    {
        ready_queue * temp_queue_entry = head_queue;
        Thread * next_thread_id = temp_queue_entry->thread_id;
        if (head_queue == tail_queue) 
        {
            tail_queue == NULL;	
        }
        // Update the head
        head_queue = head_queue->next;
        // Delete the queue entry
        delete temp_queue_entry;
        // Process next thread
        Thread::dispatch_to(next_thread_id);
    }
    // Enable the interrupt if disabled
    if (!Machine::interrupts_enabled())
        Machine::enable_interrupts();	
}

void RoundRobinScheduler::resume(Thread * _thread) 
{
    // Disable the interrupt if enabled
    if (Machine::interrupts_enabled())
        Machine::disable_interrupts();
    ready_queue * new_queue_entry = new ready_queue;
    new_queue_entry->thread_id = _thread;
    new_queue_entry->next = NULL;
    if (head_queue == NULL) 
    {
        // First entry in queue
        head_queue = new_queue_entry;
        tail_queue = new_queue_entry;
    }
    else 
    {
        // Queue already present. Add entry
        tail_queue->next = new_queue_entry;
        tail_queue = tail_queue->next;
    }
    // Enable the interrupt if disabled
    if (!Machine::interrupts_enabled())
        Machine::enable_interrupts();	
}

void RoundRobinScheduler::add(Thread * _thread) 
{

    resume (_thread);
}

void RoundRobinScheduler::terminate(Thread * _thread)
{
    delete _thread;
    yield();
}
