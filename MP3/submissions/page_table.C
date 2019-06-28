#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

PageTable * PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = NULL;
ContFramePool * PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;



void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
    kernel_mem_pool = _kernel_mem_pool;
    process_mem_pool = _process_mem_pool;
    shared_size = shared_size;
    Console::puts("Initialized Paging System\n");
}

PageTable::PageTable()
{
    // Getting a frame for page directory
    page_directory = (unsigned long *) ((kernel_mem_pool->get_frames(1)) * PAGE_SIZE);
    // Getting a frame for page table
    unsigned long *page_table = (unsigned long *) ((kernel_mem_pool->get_frames(1)) * PAGE_SIZE);
    unsigned long address = 0;
    unsigned int i, j;
    for (i = 0; i < 1024; i++)
    {
        //attribute set to: supervisor level, read/write, present (011 in binary)
        page_table[i] = address | 3; 
        address = address + 4096; //4kb	
    }
    page_directory[0] = (unsigned long) page_table;
    //attribute set to: supervisor level, read/write, present (011 in binary) 
    page_directory[0] = page_directory[0] | 3; 

    // mark rest of the page table as not present in the page directory
    for (j = 1; j < 1024; j++) 
    {
        //attribute set to: supervisor level, read/write, not present (010 in binary)
        page_directory[j] = 0 | 2;   
    }
    Console::puts("Constructed Page Table object\n");
}


void PageTable::load()
{
    current_page_table = this;
    //Store page directory address into CR3 register
    write_cr3( (unsigned long) page_directory ); 
    Console::puts("Loaded page table\n");
}

void PageTable::enable_paging()
{
    // Start paging
    write_cr0 (read_cr0() | 0x80000000);
    paging_enabled = 1;
    Console::puts("Enabled paging\n");
}

void PageTable::handle_fault(REGS * _r)
{
    unsigned long   fault_address = read_cr2();
    unsigned long * page_directory ((unsigned long *) read_cr3());
    unsigned long   page_directory_index = fault_address >> 22;
    unsigned long   page_table_index;
    unsigned long * page_table;
    unsigned long * page_table_address;
    unsigned long   page_entry;
    unsigned long * page;
    if (_r->err_code & 0xFFFFFFFE) 
    {
        // checking the present bit in page directory
        if ((page_directory[page_directory_index] & 0x1) == 0) 
        {
            // fault occured at page directory. Create a page table and update the page directory
            page_table = (unsigned long *) ((kernel_mem_pool->get_frames(1)) * PAGE_SIZE);
            //attribute set to: supervisor level, read/write, present (011 in binary) 
            page_directory[page_directory_index] = ((unsigned long) page_table) | 3;
        }
        // Get the page table address as it is the frame number of 20 bits
        page_table_address = ((unsigned long *) (page_directory[page_directory_index] & 0xFFFFF000));
        page_table_index = (fault_address >> 12 & 0x03FF);
        // checking the present bit in page table
        if ((page_table_address[page_table_index] & 0x1) == 0)
        {
            page = (unsigned long *) ((process_mem_pool->get_frames(1)) * PAGE_SIZE);
            //attribute set to: supervisor level, read/write, present (011 in binary) 
            page_table_address[page_table_index] = ((unsigned long) page) | 3;
        }

    }
    Console::puts("handled page fault\n");
}

