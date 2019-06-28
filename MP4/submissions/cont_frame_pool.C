/*
 File: ContFramePool.C
 
 Author:
 Date  : 
 
 */

/*--------------------------------------------------------------------------*/
/* 
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates 
 *single* frames at a time. Because it does allocate one frame at a time, 
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.
 
 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free 
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.
 
 This can be done in many ways, ranging from extensions to bitmaps to 
 free-lists of frames etc.
 
 IMPLEMENTATION:
 
 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame, 
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool. 
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.
 
 NOTE: If we use this scheme to allocate only single frames, then all 
 frames are marked as either FREE or HEAD-OF-SEQUENCE.
 
 NOTE: In SimpleFramePool we needed only one bit to store the state of 
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work, 
 revisit the implementation and change it to using two bits. You will get 
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.
 
 DETAILED IMPLEMENTATION:
 
 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:
 
 Constructor: Initialize all frames to FREE, except for any frames that you 
 need for the management of the frame pool, if any.
 
 get_frames(_n_frames): Traverse the "bitmap" of states and look for a 
 sequence of at least _n_frames entries that are FREE. If you find one, 
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or 
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
 
 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.
 
 needed_info_frames(_n_frames): This depends on how many bits you need 
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.
 
 A WORD ABOUT RELEASE_FRAMES():
 
 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e., 
 not associated with a particular frame pool.
 
 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete
 
 */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

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
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/

ContFramePool* ContFramePool::frame_pool_list;

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no,
                             unsigned long _n_info_frames)
{
    // Bitmap must fit in a single frame!
    assert(_n_frames <= FRAME_SIZE * 8);
    
    base_frame_no = _base_frame_no;
    nframes = _n_frames;
    nFreeFrames = _n_frames;
    info_frame_no = _info_frame_no;
    
    // If _info_frame_no is zero then we keep management info in the first
    //frame, else we use the provided frame to keep management info
    if(info_frame_no == 0) {
        bitmap = (unsigned char *) (base_frame_no * FRAME_SIZE);
    } else {
        bitmap = (unsigned char *) (info_frame_no * FRAME_SIZE);
    }
    
    // Number of frames must be "fill" the bitmap!
    assert ((nframes % 4 ) == 0);
    
    
    // Everything ok. Proceed to mark all bits in the bitmap
    for(int i=0; i*4 < _n_frames; i++) {
        bitmap[i] = 0xFF;
    }
    
    // Mark the first frame as being used if it is being used
    if(_info_frame_no == 0) {
        // Mark the first frame as allocated. Set first two bits to 00.
        bitmap[0] = 0x3F;
        nFreeFrames--;
    }
    
    if (ContFramePool::frame_pool_list==NULL)
        ContFramePool::frame_pool_list=this;
    else
        frame_pool_list->next = this;
    
     prev = frame_pool_list;
    Console::puts("Frame Pool initialized\n");
}

unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
    // Any frames left to allocate?
    assert(nFreeFrames > 0);
    //Console::puts("========== get_frames entering =========="); Console::puts("\n"); 
    
    unsigned int frame_no = base_frame_no;
    unsigned int frames_req = _n_frames;
    unsigned int first_frame_no = 0;
    unsigned int prev_frame_no = 0;
    unsigned int last_frame_no = 0;
    unsigned int head_index = 0;
    unsigned int count = 0;
    unsigned int index = 0;
    
    unsigned int i = 0, j = 0, k = 0;
    unsigned char mask = 0xC0;
    unsigned char mask_reset = 0xC0;
    unsigned char mask_get_head_frame;
    unsigned char mask_check_head_frame;
    //Console::puts("Enter frame search ");Console::puts("\n"); 
    while (i < (nframes / 4) && (count < frames_req))
    {
        for (j = 0; j < 4 && (count < frames_req); j++)
        {
            //Console::puts("bit map index ");Console::puti(i); 
            //Console::puts("  bitmap[i] ");Console::puti(bitmap[i]); 
            //Console::puts("  jth value ");Console::puti(j); Console::puts("\n"); 
            //Console::puts("mask ");Console::puti(mask); Console::puts("\n"); 
            if ((bitmap[i] & mask) == mask)
            {
                // frame is free, increase the frame count
                count++;
                last_frame_no = base_frame_no + i*4 + j;
                //Console::puts("last_frame_no ");Console::puti(last_frame_no); Console::puts("\n"); 
                //Console::puts("Found a free frame ");
                //Console::puts(" frame count ");Console::puti(count);Console::puts("\n"); 
            }
            mask_get_head_frame = 0xC0 >> ((last_frame_no%4) * 2);
            mask_check_head_frame = 0x80 >> ((last_frame_no%4) * 2);

            head_index = bitmap[i] & mask_get_head_frame;
            if (((head_index ^ mask_check_head_frame) == 0))
            {
                // frame is a head frame. reset the count value which prevents
                // non-contiguous allocation.
                //Console::puts("Non-contigous frame. Reset count to 0 \n");
                count = 0;
            }
            mask = mask >> 2;
        }
        mask = 0xC0;
        i++;
    }
    
    //Console::puts("Initializing head frame ");Console::puts("\n"); 

    // get the first frame number as you know the last frame
    first_frame_no = last_frame_no - frames_req + 1;
    //Console::puts("head frame_no ");Console::puti(first_frame_no); Console::puts("\n"); 
    mask = 0xBF >> ((first_frame_no%4) * 2);
    mask_reset = 0xC0 >> ((first_frame_no%4) * 2);
    //Console::puts("head frame mask ");Console::puti(mask); Console::puts("\n"); 

    // marking first frame as head frame in bit map
    bitmap[(first_frame_no - base_frame_no)/4] = bitmap[(first_frame_no - base_frame_no)/4] & (~mask_reset);
    bitmap[(first_frame_no - base_frame_no)/4] = bitmap[(first_frame_no - base_frame_no)/4] | mask;
    //Console::puts("value at head frame ");
    //Console::puti(bitmap[(first_frame_no - base_frame_no)/4]); Console::puts("\n"); 

    //Console::puts("\n");
    //Console::puts("Initializing rest of frames ");Console::puts("\n");
    // marking rest of frames as allocated frame in bit map
    for (k = first_frame_no + 1; k <= last_frame_no; k++)
    {
        index = (k - base_frame_no)/4;
        //Console::puts("index ");Console::puti(index); Console::puts("\n"); 
        mask = 0xC0 >> ((k%4)*2);
        //Console::puts("mask ");Console::puti(mask); Console::puts("\n"); 
        bitmap[index] = bitmap[index] & (~mask);
        //Console::puts("bitmap[index] ");Console::puti(bitmap[index]); Console::puts("\n"); 
        //Console::puts("\n");
    }
    //Console::puts("\n");

    nFreeFrames -= frames_req;


    //Console::puts("Printing all bitmap values ");Console::puts("\n"); 
    //for (j = 0; j < 32; j++)
    {
        //Console::puts("[j] ");Console::puti(j); 
        //Console::puts("  bitmap[j] ");Console::puti(bitmap[j]); Console::puts("\n"); 
    }
    //Console::puts("========== get_frames exiting =========="); Console::puts("\n"); 
    return (first_frame_no);
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
    unsigned char mask, mask_reset;
    int k, index;
    unsigned long last_inaccess_frame;

    //Console::puts("========== mark_inaccessible entering =========="); Console::puts("\n"); 
    last_inaccess_frame = _base_frame_no + _n_frames - 1;
  //  Console::puts("input base frame ");Console::puti(_base_frame_no); Console::puts("\n"); 
    //Console::puts("no of frames ");Console::puti(_n_frames); Console::puts("\n"); 
    //Console::puts("real base frame ");Console::puti(base_frame_no); Console::puts("\n"); 

    for (k = _base_frame_no; k <= last_inaccess_frame; k++)
    {
        // find the index and mark it as allocated
        index = (k - base_frame_no)/4;
       // Console::puts("index ");Console::puti(index); Console::puts("\n"); 
        mask_reset = 0xC0 >> ((k%4)*2);
        mask = 0xBF >> ((k%4) * 2);
        //Console::puts("mask ");Console::puti(mask); Console::puts("\n"); 
        bitmap[index] = bitmap[index] & (~mask_reset);
        bitmap[index] = bitmap[index] | mask;
        //Console::puts("bitmap[index] ");Console::puti(bitmap[index]); Console::puts("\n"); 
        //Console::puts("\n");
    }
    //Console::puts("========== mark_inaccessible exiting =========="); Console::puts("\n"); 

}

void ContFramePool::release_frames(unsigned long _first_frame_no)
{
    unsigned int first_frame_no = _first_frame_no;
    unsigned int next_frame_no = 0;
    unsigned int i = 0, j = 0, k = 0;
    unsigned int frame_free_count = 0;
    unsigned char mask_free;
    unsigned char mask;
    unsigned char* bitmap;
    unsigned int    nFreeFrames;   //
    unsigned long   base_frame_no; // Where does the frame pool start in phys mem?
    unsigned long   nframes;       // Size of the frame pool
    unsigned long   info_frame_no; // Where do we store the management information?

    ContFramePool* curr=ContFramePool::frame_pool_list;
    //Console::puts("========== release_frames entering =========="); Console::puts("\n"); 

    //determine if frame is in kernel pool or process pool
    if (curr->base_frame_no+ curr->nframes <= _first_frame_no)
    {
        if(curr->next!=NULL)
            curr=curr->next;
        else
        {
            Console::puts("Error releasing frame, was not contained in any frame pools\n");
            assert(false);
        }
    }

    bitmap = curr->bitmap;
    nFreeFrames = curr->nFreeFrames;   //
    base_frame_no = curr->base_frame_no; // Where does the frame pool start in phys mem?
    nframes = curr->nframes;       // Size of the frame pool
    info_frame_no = curr->info_frame_no; // Where do we store the management information?

    mask = 0x80 >> ((first_frame_no%4) * 2);
    //Console::puts("first_frame_no = "); Console::puti(first_frame_no);Console::puts("\n");  
    //Console::puts("mask = "); Console::puti(mask); Console::puts("\n"); 
    //Console::puts("bitmap[(first_frame_no - base_frame_no)/4] = "); Console::puti(bitmap[(first_frame_no - base_frame_no)/4]); Console::puts("\n"); 


    if ((bitmap[(first_frame_no - base_frame_no)/4] & mask) != mask)
    {
        Console::puts("first frame is not a head frame. Exit "); 
        assert(false);
    }


    mask = 0xC0 >> ((first_frame_no%4) * 2);
    bitmap[(first_frame_no - base_frame_no)/4] = bitmap[(first_frame_no - base_frame_no)/4] | mask;

    // start from next frame
    next_frame_no = first_frame_no + 1;
    mask = 0xC0 >> ((next_frame_no%4) * 2);
    frame_free_count++;

    //Console::puts("Looping to release other frames "); 
    i = (next_frame_no - base_frame_no)/4;
    while (i < (nframes / 4))
    {
        i = (next_frame_no - base_frame_no)/4;
        //Console::puts("next_frame_no ");Console::puti(next_frame_no); Console::puts("\n"); 
        mask = 0xC0 >> ((next_frame_no%4) * 2);
        //Console::puts("[i] ");Console::puti(i);Console::puts("\n");  
        if ((bitmap[i] & mask) == 0)
        {
            bitmap[i] = bitmap[i] | mask;
            frame_free_count++;
            //Console::puts("  bitmap[i] ");Console::puti(bitmap[i]); Console::puts("\n"); 
            //Console::puts("frame_free_count ");Console::puti(frame_free_count); Console::puts("\n"); 
        }
        else
        {
            //Console::puts("Breaking loop "); Console::puts("\n"); 
            for (j = 0; j < 32; j++)
            {
                //Console::puts("[j] ");Console::puti(j); 
                //Console::puts("  bitmap[j] ");Console::puti(bitmap[j]); Console::puts("\n"); 
            }
            break;
        }
        next_frame_no++;

    }


    nFreeFrames += frame_free_count;
    //Console::puts("========== release_frames exiting =========="); Console::puts("\n"); 

}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    return _n_frames / (16*1024) + (_n_frames % (16*1024) > 0 ? 1 : 0);
}
