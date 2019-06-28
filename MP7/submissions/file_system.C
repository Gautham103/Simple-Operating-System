/*
     File        : file_system.C

     Author      : Riccardo Bettati
     Modified    : 2017/05/01

     Description : Implementation of simple File System class.
                   Has support for numerical file identifiers.
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
#include "file_system.H"


/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

FileSystem::FileSystem() 
{
    Console::puts("In file system constructor.\n");
    memset(disk_buffer,0,BLOCK_SIZE);
}

/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/

bool FileSystem::Mount(SimpleDisk * _disk)
{
    Console::puts("mounting file system form disk\n");
    disk = _disk;
    return true;
}

bool FileSystem::Format(SimpleDisk * _disk, unsigned int _size)
{
    int i;
    memset(disk_buffer,0,BLOCK_SIZE);
    // clearing all data in buffer 
    for (i = 0; i < SYSTEM_BLOCKS; i++)
        _disk->write(i,disk_buffer);

    // Initialize 0th block which keep track file info
    _disk->read(0,disk_buffer);
    for (i = 0; i < TOTAL_BLOCKS; i++)
    {
        fs_info[i].fd = -1; 
        fs_info[i].start_block = 0; 
        fs_info[i].total_block_size = 0; 
    }
    _disk->write(0,disk_buffer);

    // Initialize 1st block which keep track free blocks
    memset(disk_buffer,0,BLOCK_SIZE);
    _disk->read(1,disk_buffer);
    for (i = 0; i < BLOCK_SIZE; i++)
    {
        disk_buffer[i] |= 0xFF;
    }

    int j;
    // Marking 0th and 1st block used
    disk_buffer[0] = 0x3F;

    _disk->write(1,disk_buffer);
    
    return true;
}

File * FileSystem::LookupFile(int _file_id) 
{
    int i;
    File* FileObj = NULL;
    
    memset(disk_buffer, 0, BLOCK_SIZE);
    disk->read (0, disk_buffer);
    // Find the matching fd in inodes
    for (i = 0; i < TOTAL_BLOCKS; i++)
    {
        if (fs_info[i].fd == _file_id)
        {
            Console::puts("File found "); Console::puti(_file_id); Console::puts("\n");
            FileObj = (File*) new File(_file_id, fs_info[i].start_block, fs_info[i].total_block_size, fs_info[i].curr_position);
            break;
        }
    }
    return FileObj; 
}

bool FileSystem::CreateFile(int _file_id) 
{
    int i, j;
    i = 0;
    int block_no = 0;
    unsigned char mask = 0x80;
    memset(disk_buffer, 0, BLOCK_SIZE);
    disk->read (1, disk_buffer);

    // Check for free blocks
    while (disk_buffer[i] == 0) {
        i++;
    }
    block_no += i * 8;

    while ((mask & disk_buffer[i]) == 0) {
        mask = mask >> 1;
        block_no++;
    }
    
    // Update bitmap
    disk_buffer[i] = disk_buffer[i] ^ mask;
    disk->write (1, disk_buffer);

    memset(disk_buffer, 0, BLOCK_SIZE);
    disk->read (0, disk_buffer);

    for (i = 0; i < TOTAL_BLOCKS; i++)
    {
        if (fs_info[i].fd == -1)
        {
            // Assign a free block to the file
            fs_info[i].fd = _file_id;
            fs_info[i].start_block = block_no;
            fs_info[i].total_block_size = 1;
            fs_info[i].curr_position = 0;
            break;
        }
    }
    disk->write(0,disk_buffer);
    return true;

}

bool FileSystem::DeleteFile(int _file_id) 
{
    int i;
    int start_blk_no;
    int total_blk_no;
    unsigned char mask = 0x80;
    memset(disk_buffer,0,BLOCK_SIZE);
    disk->read (0, disk_buffer);

    // free file in block 0
    for (i = 0; i < TOTAL_BLOCKS; i++)
    {
        if (fs_info[i].fd == _file_id)
        {
            start_blk_no = fs_info[i].start_block;
            total_blk_no = fs_info[i].total_block_size;
            fs_info[i].fd = -1;
            fs_info[i].start_block = -1; 
            fs_info[i].total_block_size = -1; 
            fs_info[i].curr_position = 0;
            break;
        }
    }
    disk->write(0,disk_buffer);

    // free the blocks
    FreeBlocks(start_blk_no, total_blk_no);

    return true;
}

int FileSystem :: GetBlocks (int NumBlocks, int fd)
{
    int i, j;
    int start_block = 0;
    int last_block = 0;
    int block_count = 0;
    unsigned char mask = 0x80;
    memset(disk_buffer,0,BLOCK_SIZE);
    disk->read (1, disk_buffer);

    for (i = 0; i < SYSTEM_BLOCKS && block_count < NumBlocks; i++)
    {
        for (j = 0; j < 8 && block_count < NumBlocks; j++)
        {
            if ((disk_buffer[i] & mask) != 0)
            {
                // free block
                last_block = i * 8 + j;
                block_count++;
            }
            else
            {
                block_count = 0;
            }
            mask = mask >> 1;
        }
        mask = 0x80;
    }
    if (NumBlocks != 1)
        start_block = last_block - NumBlocks;
    else
        start_block = last_block;

    // Mark it as used
    for (j = start_block; j <= last_block; j++)
    {
        mask = 0x80 >> (j % 8);
        disk_buffer[j / 8] ^= mask; 
    }
    disk->write(1,disk_buffer);

    // Now the file has one more block. Update the file in block 0
    memset(disk_buffer,0,BLOCK_SIZE);
    disk->read (0, disk_buffer);

    for (i = 0; i < TOTAL_BLOCKS; i++)
    {
        if (fs_info[i].fd == fd)
        {
            fs_info[i].start_block = start_block;
            fs_info[i].total_block_size = NumBlocks;
            fs_info[i].curr_position = 0;
            break;
        }
    }
    disk->write(0,disk_buffer);

    return start_block;
}

int FileSystem :: FreeBlocks(int start_block, int TotalBlocks)
{
    int i;
    unsigned char mask = 0x80;
    memset(disk_buffer,0,BLOCK_SIZE);
    disk->read (1, disk_buffer);
    // Mark the blocks as available
    for (i = start_block; i <= start_block + TotalBlocks - 1; i++)
    {
        mask = 0x80 >> (i % 8);
        disk_buffer[i / 8] |= mask;
    }

    disk->write(1,disk_buffer);
}
