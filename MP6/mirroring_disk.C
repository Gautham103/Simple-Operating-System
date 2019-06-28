/*
     File        : mirroring_disk.c

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
#include "mirroring_disk.H"
#include "scheduler.H"
#include "simple_disk.H"
#include "blocking_disk.H"
#include "thread.H"

extern Scheduler * SYSTEM_SCHEDULER;
/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

//#define ENABLE_THREAD_SYNC

#ifdef ENABLE_THREAD_SYNC 
int mutex;
int test_and_set(int *mutex) 
{
    int tmp = *mutex;
    *mutex = 1;
    return tmp;
}

void mutex_init(int *mutex) 
{
   *mutex = 0;
}

void mutex_lock() 
{
   while(test_and_set(&mutex));
}

void mutex_unlock()
{
   mutex = 0;
}
#endif

MirroringDisk::MirroringDisk(DISK_ID _disk_id, unsigned int _size): BlockingDisk(_disk_id, _size) 
{
    MASTER_DISK = new BlockingDisk(MASTER, _size);
    SLAVE_DISK = new BlockingDisk(SLAVE, _size);
#ifdef ENABLE_THREAD_SYNC 
    mutex_init(&mutex);
#endif
}

/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/



void issue_operation_mirr(DISK_OPERATION _op, unsigned long _block_no,DISK_ID disk_id)
{

  Machine::outportb(0x1F1, 0x00); /* send NULL to port 0x1F1         */
  Machine::outportb(0x1F2, 0x01); /* send sector count to port 0X1F2 */
  Machine::outportb(0x1F3, (unsigned char)_block_no);
                         /* send low 8 bits of block number */
  Machine::outportb(0x1F4, (unsigned char)(_block_no >> 8));
                         /* send next 8 bits of block number */
  Machine::outportb(0x1F5, (unsigned char)(_block_no >> 16));
                         /* send next 8 bits of block number */
  Machine::outportb(0x1F6, ((unsigned char)(_block_no >> 24)&0x0F) | 0xE0 | (disk_id << 4));
                         /* send drive indicator, some bits, 
                            highest 4 bits of block no */

  Machine::outportb(0x1F7, (_op == READ) ? 0x20 : 0x30);

}

void MirroringDisk::wait_until_ready_mirr()
{
	while (!MASTER_DISK->blocking_is_ready() || !SLAVE_DISK->blocking_is_ready()) 
    {
		SYSTEM_SCHEDULER->resume(Thread::CurrentThread());
		SYSTEM_SCHEDULER->yield();
	}
}

void MirroringDisk::read(unsigned long _block_no, unsigned char * _buf) 
{
#ifdef ENABLE_THREAD_SYNC 
    mutex_lock();
#endif
    issue_operation_mirr(READ, _block_no, MASTER);
    issue_operation_mirr(READ, _block_no, SLAVE);
    wait_until_ready_mirr();

     /* read data from port */
  int i;
  unsigned short tmpw;
  for (i = 0; i < 256; i++) {
    tmpw = Machine::inportw(0x1F0);
    _buf[i*2]   = (unsigned char)tmpw;
    _buf[i*2+1] = (unsigned char)(tmpw >> 8);
  }
#ifdef ENABLE_THREAD_SYNC 
  mutex_unlock();
#endif

}

void MirroringDisk::write(unsigned long _block_no, unsigned char * _buf) 
{
#ifdef ENABLE_THREAD_SYNC 
    mutex_lock();
#endif
    MASTER_DISK->write (_block_no, _buf);
    SLAVE_DISK->write (_block_no, _buf);
#ifdef ENABLE_THREAD_SYNC 
    mutex_unlock();
#endif
}
