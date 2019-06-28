/*
 File: vm_pool.C
 
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

#include "vm_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "page_table.H"

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
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

VMPool::VMPool(unsigned long  _base_address,
               unsigned long  _size,
               ContFramePool *_frame_pool,
               PageTable     *_page_table) 
{
    int i;
    // Initialize all the data structures
    base_address = _base_address;
    size = _size;
    frame_pool = _frame_pool;
    page_table = _page_table;
    page_table -> register_pool(this);

    memory_region * region_list = (memory_region *)base_address;
    region_list[0].start_address = _base_address;
    region_list[0].size = Machine::PAGE_SIZE;
    for (i = 1; i < 512; i++)
    {
        region_list[i].start_address = 0;
        region_list[i].size = 0;
    }

    Console::puts("Constructed VMPool object.\n");
}

unsigned long VMPool::allocate(unsigned long _size) 
{
    int i;
    int allocated_size = Machine::PAGE_SIZE;
    
    if (_size > Machine::PAGE_SIZE)
        allocated_size = Machine::PAGE_SIZE * (_size/Machine::PAGE_SIZE) + ((_size % Machine::PAGE_SIZE == 0) ? 0 : Machine::PAGE_SIZE);

    Console::puts("========== allocate entering =========="); Console::puts("\n"); 
    memory_region * region_list = (memory_region *)base_address;
    for (i = 1; i < 512; i++)
    {
        // Find the free region and allocate the memory
        if (region_list[i].size == 0)
        {
            region_list[i].start_address = region_list[i-1].start_address + region_list[i-1].size;
            region_list[i].size = allocated_size;
            break;
        }
    }

    Console::puts("Allocated region of memory.\n");
    Console::puts("========== allocate exiting =========="); Console::puts("\n"); 
    return region_list[i].start_address;
}

void VMPool::release(unsigned long _start_address)
{
    Console::puts("========== release entering =========="); Console::puts("\n"); 
    memory_region * region_list = (memory_region *)base_address;
    int i,j;
    // Find the region corresponding to the address
    for (i = 1; i < 512; i++)
    {
        if (region_list[i].start_address == _start_address)
        {
            Console::puts("Found the address"); Console::puts("\n"); 
            break;
        }

    }

    int k;

    // Free the frames allocated
    for (k = Machine::PAGE_SIZE; k <= region_list[i].size; k += Machine::PAGE_SIZE)
    page_table -> free_page(region_list[i].start_address + k);

    while (i < 511)
    {
        if (region_list[i + 1].size != 0)
        {
            region_list[i].start_address = region_list[i + 1].start_address;
            region_list[i].size = region_list[i + 1].size;
        }
        else
        {
            region_list[i].start_address = 0;
            region_list[i].size = 0;
        }
        i++;
    }


    Console::puts("Released region of memory.\n");
    Console::puts("========== release exiting =========="); Console::puts("\n"); 
}

bool VMPool::is_legitimate(unsigned long _address) 
{
    if (_address == base_address)
    {
        return true;
    }
	int i;
    int flag = 0;
    memory_region * region_list = (memory_region *) (base_address);
	
    // Check whether the address falls into a region
    for (i = 0; i < 512; i++)
    {
        if ((_address >= region_list[i].start_address) && (_address <= region_list[i].start_address + region_list[i].size))
        {
            flag = 1;
            break;
        }
	}

	if (flag == 1)
    {
		return true;
    }
    else 
    {
		return false;
    }
    
    Console::puts("Checked whether address is part of an allocated region.\n");
}

