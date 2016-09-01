/*--------------------------------------------------------------------------------------------------
 * Author:      
 * Date:        2016-06-26
 * Assignment:  Final Project
 * Source File: vdi_reader.cpp
 * Language:    C/C++
 * Course:      Operating Systems
 * Purpose:     Contains the implementation of the vdi_reader class.
 -------------------------------------------------------------------------------------------------*/

#include "vdi_reader.h"
#include "datatypes.h"
#include <iostream>
#include <iomanip>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>

//#define DEBUG_VDI_WRITE_DISABLED
#define DEBUG_VDI_OUTPUT_TRANSLATION

/* VDI reader should read the VDI header and populate the VDI header structure,
   and read the VDI Map into pageMap*/

using namespace std;

namespace vdi_explorer{
    /*----------------------------------------------------------------------------------------------
     * Name:    vdi_reader
     * Type:    Function
     * Purpose: Constructor for the vdi_reader class.
     * Input:   std::string fs, containing the file name to open.  Also prints out some debug
     *          information currently.
     * Output:  Nothing.
    ----------------------------------------------------------------------------------------------*/
    vdi_reader::vdi_reader(string fs)
    {
        vdiOpen(fs);
        
        // Debug info.
        cout << "VDI Header Information:" << endl;
        cout << "Dynamic/Static: " << (hdr.imageType == 1 ? "1 - dynamic" : "2 - static") << endl;
        cout << "MBR Start Offset: " << hdr.offsetData << endl; //2097152 - Where MBR is located
        cout << "Magic Number: " << hdr.magic << endl; // Image Signature
        cout << "Header Size: " << hdr.headerSize << endl; 
        cout << "Offset Pages: " << hdr.offsetPages << endl; 
        cout << "Disk Size: " << hdr.diskSize << endl; 
        cout << "Sector Size: " << hdr.sectorSize<< endl; 
        cout << "Page Size: " << hdr.pageSize<< endl; 
        cout << "Page Extra: " << hdr.pageExtra<< endl; 
        cout << "Total Pages: " << hdr.totalPages<< endl; // offsetBlocks and blocksInHDD
        cout << "Pages Allocated: " << hdr.pagesAllocated << endl;
        
        cout << endl;
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    ~vdi_reader
     * Type:    Function
     * Purpose: Destructor for the vdi_reader class.
     * Input:   Nothing.
     * Output:  Nothing.
    ----------------------------------------------------------------------------------------------*/
    vdi_reader::~vdi_reader(){
        vdiClose();
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    vdiOpen
     * Type:    Function
     * Purpose: Attempts to opens a VDI file, read the header, and allocate all the necessary
     *          variables.
     * Input:   const std::string fileName, holds the filename to be read.
     * Output:  Nothing.
     
     * @TODO    Magic numbers >> consts
     * @TODO    More comments.
     * @TODO    Make actual exceptions, not simply generic ones.
    ----------------------------------------------------------------------------------------------*/
    void vdi_reader::vdiOpen(const std::string fileName)
    {
        // Open the file and read the header.
        //
        // Once the VDIFile structure has been allocated, a field is available to hold the file
        // descriptor. An attempt is made to open the file; if successful, the VDI header is loaded.
        //
        // If the open fails for any reason or if the VDI header has the wrong magic number, then
        // the VDIFile structure is deallocated and NULL is returned.
        
        // Open the file and verify that it was opened successfully.
        fd = ::open(fileName.c_str(), O_RDWR);
        if (fd == -1)
        {
            cout << "Error opening file.\n";
            throw;
        }
        
        // Read the header, then verify its size and signature.
        size_t nBytes = read(fd, &hdr, sizeof(VDIHeader));
        if (nBytes != sizeof(VDIHeader) || hdr.magic != 0xbeda107f)
        {
            cout << "Unexpected header size or the magic number has lost its magic.\n";
            throw;
        }
        
        // Allocate the page map and bitmaps.
        // 
        // The page map is allocated dynamically, since the size isn’t known until the VDI header is
        // loaded.
        //
        // Since the page map may be quite large, it is loaded on an as-needed basis. The map is
        // divided into 4KB chunks with each chunk holding 4096/4 = 1024 map entries. A chunk is
        // loaded into the map the first time any entry within the chunk is needed.
        //
        // A bitmap is allocated to keep track of which chunks have been loaded. A second bitmap is
        // allocated to keep track of which chunks have been modified. A chunk can be modified if
        // one of its entries changes, which occurs when a previously unallocated page is allocated.
        
        // Allocate the page map and verify it allocated correctly.
        pageMap = new s32[hdr.totalPages];
        if (pageMap == nullptr)
        {
            ::close(fd);
            cout << "Error allocating pageMap.\n";
            throw;
        }
        
        // Allocated the page bitmap and verify it allocated correctly.
        u32 bitmapSize = (hdr.totalPages + 8191) / 8192;
        pageBitmap = new u8(bitmapSize);
        if (pageBitmap == nullptr)
        {
            ::close(fd);
            delete[] pageMap;
            pageMap = nullptr;
            cout << "Error allocating pageBitmap.\n";
            throw;
        }
        
        // Allocate the dirty bitmap and verify it allocated correctly.
        dirtyBitmap = new u8(bitmapSize);
        if (dirtyBitmap == nullptr)
        {
            ::close(fd);
            delete[] pageBitmap;
            delete[] pageMap;
            pageBitmap = nullptr;
            pageMap = nullptr;
            cout << "Error allocating dirtyBitmap.\n";
            throw;
        }
        
        // Initialize the remaining fields.
        //
        // All that remains in vdiOpen is to make sure that the fields are allocated properly
        // initialized. At this point, the file has been opened and the header has been loaded, so
        // fd and hdr are initialized. The page map does not need to be initialized; entries will
        // not be seen until their chunk has been loaded. The page and dirty bitmaps must be cleared
        // to indicate that no chunks have been loaded or modified. Finally, the cursor must be set
        // to 0.
        
        // Fill the page and dirty bitmaps with 0's.
        ::memset(pageBitmap, 0, bitmapSize);
        ::memset(dirtyBitmap, 0, bitmapSize);
        
        // Set the cursor to its base position.
        cursor = 0;
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    vdiClose
     * Type:    Function
     * Purpose: Determines if the VDI header has been modified, writes it back to the file if it has
     *          been, then closes the VDI file and deallocates the supporting variables.
     * Input:   Nothing.
     * Output:  Nothing.
     
     * @TODO:   Turn the magic numbers into constants.
    ----------------------------------------------------------------------------------------------*/
    void vdi_reader::vdiClose()
    {
        // Keep track of whether the header needs to be written.
        bool mustWriteHeader = false;
        
        // Determine the size of the dirty bitmap.
        u32 bitmapSize = (hdr.totalPages + 8191) / 8192;
        size_t chunkSize;
        
        // Scan through dirtyBitmap and write any modified chunks of the page map to disk.
        // @TODO change the name of 'i' to something a little more descriptive.
        for (u32 i = 0; i < bitmapSize; i++)
        {
            // @TODO Add comment to describe exactly what this line does.
            if (dirtyBitmap[i / 8] & (1 << (i % 8)))
            {
                // Calculate the chunk size and clamp it to 4096 if necessary.
                chunkSize = (hdr.totalPages - i * 1024) * sizeof(s32);
                if (chunkSize > 4096)
                {
                    chunkSize = 4096;
                }
                
                // Write the updated page map to the VDI and set the flag so the header will be
                // written as well.
                #ifndef DEBUG_VDI_WRITE_DISABLED
                ::lseek(fd, hdr.offsetPages + i * 4096, SEEK_SET);
                ::write(fd, pageMap + i * 1024, chunkSize);
                #endif
                mustWriteHeader = true;
            }
        }
        
        // If the header needs to be written to disk, do it.
        if (mustWriteHeader)
        {
            #ifndef DEBUG_VDI_WRITE_DISABLED
            ::lseek(fd, 0, SEEK_SET);
            ::write(fd, &hdr, sizeof(VDIHeader));
            #endif
        }
        
        // Close the file descriptor and deallocate variables.
        ::close(fd);
        if (dirtyBitmap)
            delete[] dirtyBitmap;
        if (pageBitmap)
            delete[] pageBitmap;
        if (pageMap)
            delete[] pageMap;
        dirtyBitmap = nullptr;
        pageBitmap = nullptr;
        pageMap = nullptr;
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    vdiSeek
     * Type:    Function
     * Purpose: Moves the file cursor to the specified place within the VDI file's virtual disk.
     * Input:   off_t offset, offset with respect to anchor.
     * Input:   int anchor, a reference point. 
     * Output:  off_t, returns location of cursor.
    ----------------------------------------------------------------------------------------------*/
    off_t vdi_reader::vdiSeek (off_t offset , int anchor)
    {
        //Moves the file cursor to the specified place within the VDI file's virtual disk.
        /*
         * The anchor is a reference point. vdiSeek sets the cursor to anchorPoint + offset, where
         * anchorPoint is 0 for SEEK_SET, the current value of cursor for SEEK_CUR and the disk size
         * (in the VDI header) for SEEK_END.
         */
        off_t location;
        if (anchor == SEEK_SET)
            location = offset;
        else if (anchor == SEEK_CUR)
            location = cursor + offset;
        else 
            location = hdr.diskSize + offset;
        if (location < 0 || (u64)location >= hdr.diskSize)
            location = -1;
        else
            cursor = location;
        return location;
        
        //cursor = hdr.offsetPages + anchor + offset;
        //return location;
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    vdiRead
     * Type:    Function
     * Purpose: Reads a certain number of bytes into a buffer.
     * Input:   void *buf, the buffer into which the data should be read.
     * Input:   size_t count, the number of bytes which should be read.
     * Output:  size_t, holding the number of bytes actually read into the buffer.
    ----------------------------------------------------------------------------------------------*/
    size_t vdi_reader::vdiRead(void *buf, size_t count)
    {
        off_t location;
        size_t chunkSize, nBytes = 0;
        
        // Determine the size of the first chunk.
        chunkSize = hdr.pageSize - cursor % hdr.pageSize;
        if (chunkSize > count)
        {
            chunkSize = count;
        }
        
        while (count > 0)
        {
            // Read a chunk.
            location = vdiTranslate();
            if (location == 0)
            {
                for (u32 i = 0; i < chunkSize; i++)
                {
                    ((u8 *)buf)[nBytes + i] = '\0';
                }
            }
            else
            {
                ::lseek(fd, location, SEEK_SET);
                ::read(fd, ((u8 *)buf) + nBytes, chunkSize);
            }
            // Update the cursor to the new position, augment the number of bytes read, and reduce
            // the number of bytes yet to be read.
            cursor += chunkSize;
            nBytes += chunkSize;
            count -= chunkSize;
            
            // Get the size of the next chunk.
            chunkSize = hdr.pageSize;
            if (chunkSize > count)
            {
                chunkSize = count;
            }
        }
        
        // Return the number of bytes read.
        return nBytes;
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    vdiWrite
     * Type:    Function
     * Purpose: Writes a number of bytes to the VDI file from a buffer.
     * Input:   const void *buf, contains the data to be written to the VDI file.
     * Input:   size_t count, holds the number of bytes to be written to the VDI file.
     * Output:  size_t, holds the number of bytes written.
    ----------------------------------------------------------------------------------------------*/
    size_t vdi_reader::vdiWrite(const void *buf, size_t count)
    {
        off_t location;
        size_t chunkSize, nBytes = 0;
        
        // Determine the size of the first chunk.
        chunkSize = hdr.pageSize - cursor % hdr.pageSize;
        if (chunkSize > count)
        {
            chunkSize = count;
        }
        
        // Write the contents of the buffer, one chunk at a time.
        while (count > 0)
        {
            // Write a chunk.
            location = vdiTranslate();
            
            // Check if the specified location is allocated.
            if (location == 0)
            {
                // If the specified location has not been allocated yet, do so.
                vdiAllocatePageFrame();
                location = vdiTranslate();
                
                // Check again that the specified location is allocated.
                if (location == 0)
                {
                    // If the specified location has still not been allocated, A Bad Thing has
                    // happened.  Break the loop.
                    break;
                }
            }
            #ifndef DEBUG_VDI_WRITE_DISABLED
            ::lseek(fd, location, SEEK_SET);
            ::write(fd, ((u8 *)buf) + nBytes, chunkSize);
            #endif
            // Update the cursor to the new position, augment the number of bytes written, and
            // reduce the number of bytes yet to be written.
            cursor += chunkSize;
            nBytes += chunkSize;
            count -= chunkSize;
            
            // Get the size of the next chunk.
            chunkSize = hdr.pageSize;
            if (chunkSize > count)
            {
                chunkSize = count;
            }
        }
        
        // Return the number of bytes written.
        return nBytes;
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    vdiTranslate
     * Type:    Function
     * Purpose: Performs the virtual-to-physical translation.
     * Input:   Nothing.
     * Output:  off_t, holds the offset to the actual data on disk.
     
     * @TODO    Yet again, turn the magic numbers into well-named constants.
    ----------------------------------------------------------------------------------------------*/
    off_t vdi_reader::vdiTranslate()
    {
        u32 pageNum;
        off_t offset;
        
        // Check to make sure the cursor is pointing to somewhere valid on the vitual disk.
        if ((u64)cursor >= hdr.diskSize)
        {
            return 0;
        }
        
        // Compute the page number and offset.
        // @TODO look into creating a function to perform this as a bitshift.
        pageNum = cursor / hdr.pageSize;
        offset = cursor - pageNum * hdr.pageSize;
        
        // Load page map chunk if necessary.
        // Check if the page map chunk is loaded.
        u32 chunkNum = pageNum / 1024;
        if ((pageBitmap[chunkNum / 8] & (1 << (chunkNum % 8))) == 0)
        {
            // Calculate the chunk size and clamp it to 4096 if exceeded.
            size_t chunkSize = (hdr.totalPages - chunkNum * 1024) * sizeof(s32);
            if (chunkSize > 4096)
            {
                chunkSize = 4096;
            }
            
            // Read the page map chunk from disk.
            ::lseek(fd, hdr.offsetPages + 4096 * chunkNum, SEEK_SET);
            ::read(fd, pageMap + 1024 * chunkNum, chunkSize);
            pageBitmap[chunkNum / 8] |= ( 1 << (chunkNum % 8));
        }
        
        // Handle (return 0 for) unallocated pages.
        if (pageMap[pageNum] == -1)
        {
            return 0;
        }
        
        // Do actual virtual to physical translation.
        offset += pageMap[pageNum] * hdr.pageSize + hdr.offsetData;
        
        #ifdef DEBUG_VDI_OUTPUT_TRANSLATION
        cout << "VDI Translation Offset: " << offset << endl;
        #endif
        
        return offset;
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    vdiAllocatePageFrame
     * Type:    Function
     * Purpose: Allocates a new page frame in the VDI file.
     * Input:   Nothing.
     * Output:  Nothing.
     
     * @TODO    Turn the magic numbers into constants.
     * @TODO    More and better comments.  Explain exactly what's going on in this function.
    ----------------------------------------------------------------------------------------------*/
    void vdi_reader::vdiAllocatePageFrame()
    {
        // Add page frame.
        u8 *tmpBuffer = new u8[hdr.pageSize];
        ::memset(tmpBuffer, 0, hdr.pageSize);
        #ifndef DEBUG_VDI_WRITE_DISABLED
        ::lseek(fd, 0, SEEK_END);
        ::write(fd, tmpBuffer, hdr.pageSize);
        #endif
        delete[] tmpBuffer;
        
        // Update the page map.
        u32 pageNum = cursor / hdr.pageSize;
        pageMap[pageNum] = hdr.pagesAllocated;
        hdr.pagesAllocated++;
        u32 chunkNum = pageNum / 1024;
        dirtyBitmap[chunkNum / 8] |= (1 << (chunkNum % 8));
    }
} // namespace vdi_explorer