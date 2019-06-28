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
VMPool * PageTable::vm_pool_list[];



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
    // Getting a frame for page directory from process frame pool
    page_directory = (unsigned long *) ((process_mem_pool->get_frames(1)) * PAGE_SIZE);
    // Getting a frame for page table from process frame pool
    unsigned long *page_table = (unsigned long *) ((process_mem_pool->get_frames(1)) * PAGE_SIZE);
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
    // Pointing last index to the page directory
    page_directory[1023] = (unsigned long) page_directory | 3;
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
    unsigned long * page_directory = ((unsigned long *) read_cr3());
    unsigned long * page_directory_entry;
    unsigned long * page_table_entry;
    unsigned long   page_directory_index = fault_address >> 22;
    unsigned long   page_table_index;
    unsigned long * page_table;
    unsigned long * page_table_address;
    unsigned long   page_entry;
    unsigned long * page;

    unsigned int flag = 0;
	unsigned int pool_count = 0;
    int i;
    // Get the vm pool count
    for (i = 0; i < 512; i++)
    {
        if (vm_pool_list[i] != 0)
            pool_count++;
    }

	for (i = 0; i < pool_count; i++) 
    {
        // Check the address the legitimate before procedding 
		if (vm_pool_list[i] -> is_legitimate(fault_address) == true) 
        {
			flag = 1;
			break;
		}
	}
	if (flag == 0)
    {
		Console::puts("Error, Address is not legitimate \n");
		assert(false);
	}

    if (_r->err_code & 0xFFFFFFFE) 
    {
        // get the page directory entry - 1023 | 1023 | X | 00
        page_directory_entry = (unsigned long *)((0xFFFFF000 | (fault_address >> 20)) & 0xFFFFFFFC);
        // checking the present bit in page directory
        if ((*page_directory_entry & 0x1) == 0) 
        {
            // fault occured at page directory. Create a page table and update the page directory
            page_table = (unsigned long *) ((process_mem_pool->get_frames(1)) * PAGE_SIZE);
            //attribute set to: supervisor level, read/write, present (011 in binary) 
            *page_directory_entry = ((unsigned long) page_table) | 3;
        }
        // get the page table entry - 1023 | X | Y | 00
        page_table_entry = (unsigned long *)((0xFFC00000 | (((fault_address >> 10) & 0x003FF000) | ((fault_address >> 10) & 0x00000FFC))) & 0xFFFFFFFC);
        // checking the present bit in page table
        if ((*page_table_entry & 0x1) == 0)
        {
            page = (unsigned long *) ((process_mem_pool->get_frames(1)) * PAGE_SIZE);
            //attribute set to: supervisor level, read/write, present (011 in binary) 
            *page_table_entry = ((unsigned long) page) | 3;
        }

    }
    Console::puts("handled page fault\n");
}

void PageTable::register_pool(VMPool * _vm_pool)
{
    int i;
    for (i = 0; i < 512; i++)
    {
        if (vm_pool_list[i] == 0)
            break;
    }
    // keep track of vm pool list
    vm_pool_list[i] = _vm_pool;
    Console::puts("registered VM pool\n");
}

void PageTable::free_page(unsigned long _page_no) 
{
    unsigned long * page_table_entry;
    unsigned long   frame_number;
    page_table_entry = (unsigned long *)((0xFFC00000 | (((_page_no >> 10) & 0x003FF000) | ((_page_no >> 10) & 0x00000FFC))) & 0xFFFFFFFC);
    if ((*page_table_entry & 0x1))
    {
        // Release the allocated frames
        frame_number = *page_table_entry >> 12;
        process_mem_pool->release_frames(frame_number);
        // Update the page table entry
        *page_table_entry = 0 | 2;
    }
    // Reload CR3 register to flush the TLB entries
    write_cr3((unsigned long) page_directory);
    Console::puts("freed page\n");
}
