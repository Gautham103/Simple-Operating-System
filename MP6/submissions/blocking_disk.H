/*
     File        : blocking_disk.H

     Author      : 

     Date        : 
     Description : 

*/

#ifndef _BLOCKING_DISK_H_
#define _BLOCKING_DISK_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "simple_disk.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* B l o c k i n g D i s k  */
/*--------------------------------------------------------------------------*/

//#define INTERRUPTS_ENABLED
#include "thread.H"
#include "interrupts.H"
class BlockingDisk : public SimpleDisk {
#ifdef INTERRUPTS_ENABLED
    typedef struct queue 
    {
        // Thread id
        Thread * thread_id;
        // Pointer to next queue
        struct queue * next;
    }ready_queue;

    // Head queue
    ready_queue * head_queue;
    // Tail queue
    ready_queue * tail_queue;
    Thread * pop();
   void push(Thread * _thread);
   void handle_interrupt(REGS *_r);
#endif

protected:
	virtual void wait_until_ready();

public:
     virtual bool blocking_is_ready();
   BlockingDisk(DISK_ID _disk_id, unsigned int _size); 
   /* Creates a BlockingDisk device with the given size connected to the 
      MASTER or SLAVE slot of the primary ATA controller.
      NOTE: We are passing the _size argument out of laziness. 
      In a real system, we would infer this information from the 
      disk controller. */

   /* DISK OPERATIONS */

  // virtual void read(unsigned long _block_no, unsigned char * _buf);
   /* Reads 512 Bytes from the given block of the disk and copies them 
      to the given buffer. No error check! */

   //virtual void write(unsigned long _block_no, unsigned char * _buf);
   /* Writes 512 Bytes from the buffer to the given block on the disk. */

};

#endif
