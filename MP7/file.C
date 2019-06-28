/*
     File        : file.C

     Author      : Riccardo Bettati
     Modified    : 2017/05/01

     Description : Implementation of simple File class, with support for
                   sequential read/write operations.
*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file.H"
#include "file_system.H"
extern FileSystem* FILE_SYSTEM;


/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

File::File() {
    /* We will need some arguments for the constructor, maybe pointer to disk
     block with file management and allocation data. */
    Console::puts("In file constructor.\n");
    assert(false);
}

File::File (int fd, int _start_block, int _total_block, int _curr_position)
{
    Console::puts("========= File Constructor Entered ========= "); Console::puts("\n");
    // Initialize all data variables
    file_descriptor = fd;
    current_position = _curr_position;
    total_block = _total_block;
    start_block = _start_block;
    current_block = start_block;
    Console::puts("========= File Constructor Exited ========= "); Console::puts("\n");
}

/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/

int File::Read(unsigned int _n, char * _buf) 
{
    Console::puts("========= Read Entered ========= "); Console::puts("\n");
    int size = _n;
    int length = 0;
    unsigned char buffer [512];
    int i;
    int bytesRead;

    do
    {
        memset(buffer, 0, BLOCK_SIZE);
        Console::puts("fd: "); Console::puti(file_descriptor); Console::puts(" Reading block "); Console::puti(current_block); Console::puts("\n");
        FILE_SYSTEM->disk->read (current_block, buffer);
        Console::puts("fd: "); Console::puti(file_descriptor); Console::puts(" buffer  "); Console::puts((const char *)buffer); Console::puts("\n");
        for (i = 0; i < size; i++)
        {
            // read one byte at a time
            memcpy (_buf++, buffer + current_position++, 1);
            bytesRead++;
            // read from next block if the pointer is at the end
            if (current_position == BLOCK_SIZE - 1)
            {
                current_position = 0;
                current_block++;
                break;
            }
        }
        size = size - bytesRead;
    }while (size > 0);
    Console::puts("========= Read Exited ========= "); Console::puts("\n");
    return _n;

}


void File::Write(unsigned int _n, const char * _buf) 
{
    Console::puts("========= Write Entered ========= "); Console::puts("\n");
    int size = _n;
    unsigned char buffer [512];
    int new_start_block;
    int new_curr_block;
    int i;
    int bytesToWrite;
    memset(buffer, 0, BLOCK_SIZE);
    if (size > BLOCK_SIZE - current_position - 1)
    {
        bytesToWrite = BLOCK_SIZE - current_position - 1;
        memcpy (buffer + current_position, _buf, bytesToWrite);
        FILE_SYSTEM->disk->write (current_block, buffer + current_position);

        // We cannot accomodate in current block. We need one more block.
        new_start_block = FILE_SYSTEM->GetBlocks (total_block + 1, file_descriptor);
        new_curr_block = new_start_block;

        // copy data from old blocks to newly requested block
        for (i = start_block; i <= start_block + total_block - 1; i++)
        {
            memset(buffer, 0, BLOCK_SIZE);
            FILE_SYSTEM->disk->read (i, buffer);
            FILE_SYSTEM->disk->write (new_curr_block++, buffer);
        }
        // Free the old blocks
        FILE_SYSTEM->FreeBlocks (start_block, total_block);

        // update meta data
        total_block = total_block + 1;
        current_position = 0;
        current_block = new_curr_block;
        current_block = new_start_block + current_block - start_block; 
        start_block = new_start_block;

        // copy remaining data to new block
        bytesToWrite = size - bytesToWrite;
        memcpy (buffer + current_position, _buf, bytesToWrite);
        FILE_SYSTEM->disk->write (current_block, buffer + current_position);
        current_position = current_position + bytesToWrite;
    }
    else
    {
        // copy data to new block
        Console::puts("fd: "); Console::puti(file_descriptor); Console::puts(" Writing block "); Console::puti(current_block); Console::puts("\n");
        memcpy (buffer + current_position, _buf, size);
        FILE_SYSTEM->disk->write (current_block, buffer + current_position);
        current_position = current_position + size;
    }
    Console::puts("========= Write Exited ========= "); Console::puts("\n");
}

void File::Reset() 
{
    Console::puts("========= Reset Entered ========= "); Console::puts("\n");
    // reset current position
    current_position = 0;
    Console::puts("========= Reset Exited ========= "); Console::puts("\n");
    
}

void File::Rewrite() 
{
    Console::puts("========= Rewrite Entered ========= "); Console::puts("\n");
    int i;
    unsigned char buffer [512];
    memset(buffer, 0, BLOCK_SIZE);

    // clear all the data in the blocks
    for (i = start_block; i <= start_block + total_block; i++)
    {
        FILE_SYSTEM->disk->write (i, buffer);
    }
    // free the blocks
    FILE_SYSTEM->FreeBlocks (start_block, total_block);
    total_block = 1;
    // get one block for this file by default
    start_block = FILE_SYSTEM->GetBlocks (total_block, file_descriptor);
    current_block = start_block;
    current_position = 0;
    Console::puts("========= Rewrite Exited ========= "); Console::puts("\n");
}


bool File::EoF() 
{
    Console::puts("testing end-of-file condition\n");
    if (current_position == BLOCK_SIZE - 1)
        return true;
    else 
        return false;
}
