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
    head_queue = NULL;
    tail_queue = NULL;
  Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() 
{
    Console::puts("======= yield entering ======"); Console::puts("\n");
    if (Machine::interrupts_enabled())
        Machine::disable_interrupts();
        
    if (head_queue != NULL)
    {
        ready_queue *temp_queue_entry = head_queue;
        Thread * next_thread_id = head_queue->thread_id;
        head_queue = head_queue->next;
        Console::puts("Head thread_id: "); Console::puti((int)head_queue->thread_id); Console::puts("\n");
        if (head_queue == NULL)
            tail_queue = NULL;

        delete temp_queue_entry;
        Thread::dispatch_to(next_thread_id);
    }
    if (!Machine::interrupts_enabled())
        Machine::enable_interrupts();
    Console::puts("======= yield exiting ======"); Console::puts("\n");
}

void Scheduler::resume(Thread * _thread) 
{
    Console::puts("======= resume entering ======"); Console::puts("\n");
        Console::puts("head_queue: "); Console::puti((int)head_queue); Console::puts("\n");
    //Machine::disable_interrupts();
    if (head_queue == NULL || tail_queue == NULL)
    {
        Console::puts("First head node"); Console::puts("\n");
        // First entry in queue
       ready_queue *new_queue_entry = new ready_queue;
       new_queue_entry->thread_id = _thread;
       head_queue = tail_queue = new_queue_entry;
       new_queue_entry->next = NULL;
        Console::puts("head thread_id: "); Console::puti((int)new_queue_entry->thread_id); Console::puts("\n");
    }
    else
    {
        // Queue already present. Add entry
        Console::puts("Next node"); Console::puts("\n");
       ready_queue *new_queue_entry = new ready_queue;
       new_queue_entry->thread_id = _thread;
       tail_queue->next = new_queue_entry;
       tail_queue = new_queue_entry;
        Console::puts("Next thread_id: "); Console::puti((int)new_queue_entry->thread_id); Console::puts("\n");
       new_queue_entry->next == NULL;
    }
    Console::puts("======= resume exiting ======"); Console::puts("\n");
   // Machine::enable_interrupts();
}

void Scheduler::add(Thread * _thread)
{
    Console::puts("======= add entering ======"); Console::puts("\n");
    resume (_thread);
    Console::puts("======= add exiting ======"); Console::puts("\n");
}

void Scheduler::terminate(Thread * _thread) 
{
    Console::puts("======= terminate entering ======"); Console::puts("\n");
    delete _thread;
    yield ();
    Console::puts("======= terminate exiting ======"); Console::puts("\n");
}


RoundRobinScheduler::RoundRobinScheduler(unsigned int EOQ)
{
    head_queue = NULL;
    tail_queue = NULL;
    SimpleTimer * timer = new SimpleTimer(100/EOQ);
    InterruptHandler::register_handler(0, timer);
  Console::puts("Constructed RoundRobinScheduler.\n");
}

void RoundRobinScheduler::yield() 
{
    Console::puts("======= yield entering ======"); Console::puts("\n");
    if (Machine::interrupts_enabled())
    {
        Console::puts("Disabling interrupts"); Console::puts("\n");
        Machine::disable_interrupts();
    }
        
    if (head_queue != NULL)
    {
        ready_queue *temp_queue_entry = head_queue;
        Thread * next_thread_id = head_queue->thread_id;
        head_queue = head_queue->next;
        Console::puts("Head thread_id: "); Console::puti((int)head_queue->thread_id); Console::puts("\n");
        if (head_queue == NULL)
            tail_queue = NULL;

        delete temp_queue_entry;
        Thread::dispatch_to(next_thread_id);
    }
    if (!Machine::interrupts_enabled())
    {
        Console::puts("enabling interrupts"); Console::puts("\n");
        Machine::enable_interrupts();
    }
    Console::puts("======= yield exiting ======"); Console::puts("\n");
}

void RoundRobinScheduler::resume(Thread * _thread) 
{
    Console::puts("======= resume entering ======"); Console::puts("\n");
        Console::puts("head_queue: "); Console::puti((int)head_queue); Console::puts("\n");
    if (Machine::interrupts_enabled())
    {
        Console::puts("Disabling interrupts"); Console::puts("\n");
        Machine::disable_interrupts();
    }
    if (head_queue == NULL || tail_queue == NULL)
    {
        Console::puts("First head node"); Console::puts("\n");
        // First entry in queue
       ready_queue *new_queue_entry = new ready_queue;
       new_queue_entry->thread_id = _thread;
       head_queue = tail_queue = new_queue_entry;
       new_queue_entry->next = NULL;
        Console::puts("head thread_id: "); Console::puti((int)new_queue_entry->thread_id); Console::puts("\n");
    }
    else
    {
        // Queue already present. Add entry
        Console::puts("Next node"); Console::puts("\n");
       ready_queue *new_queue_entry = new ready_queue;
       new_queue_entry->thread_id = _thread;
       tail_queue->next = new_queue_entry;
       tail_queue = new_queue_entry;
        Console::puts("Next thread_id: "); Console::puti((int)new_queue_entry->thread_id); Console::puts("\n");
       new_queue_entry->next == NULL;
    }
    if (!Machine::interrupts_enabled())
    {
        Console::puts("enabling interrupts"); Console::puts("\n");
        Machine::enable_interrupts();
    }
    Console::puts("======= resume exiting ======"); Console::puts("\n");
}

void RoundRobinScheduler::add(Thread * _thread)
{
    Console::puts("======= add entering ======"); Console::puts("\n");
        Console::puts("head_queue: "); Console::puti((int)head_queue); Console::puts("\n");
    if (Machine::interrupts_enabled())
        Machine::disable_interrupts();
    if (head_queue == NULL || tail_queue == NULL)
    {
        Console::puts("First head node"); Console::puts("\n");
        // First entry in queue
       ready_queue *new_queue_entry = new ready_queue;
       new_queue_entry->thread_id = _thread;
       head_queue = tail_queue = new_queue_entry;
       new_queue_entry->next = NULL;
        Console::puts("head thread_id: "); Console::puti((int)new_queue_entry->thread_id); Console::puts("\n");
    }
    else
    {
        // Queue already present. Add entry
        Console::puts("Next node"); Console::puts("\n");
       ready_queue *new_queue_entry = new ready_queue;
       new_queue_entry->thread_id = _thread;
       tail_queue->next = new_queue_entry;
       tail_queue = new_queue_entry;
        Console::puts("Next thread_id: "); Console::puti((int)new_queue_entry->thread_id); Console::puts("\n");
       new_queue_entry->next == NULL;
    }
    if (!Machine::interrupts_enabled())
        Machine::enable_interrupts();
    Console::puts("======= add exiting ======"); Console::puts("\n");
}

void RoundRobinScheduler::terminate(Thread * _thread) 
{
    Console::puts("======= terminate entering ======"); Console::puts("\n");
    delete _thread;
    yield ();
    Console::puts("======= terminate exiting ======"); Console::puts("\n");
}
