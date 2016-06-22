#ifndef VDI_READER_H
#define VDI_READER_H

#include "datatypes.h" //typedefs for s8, u8, s16, u16, s32, u32, s64, and u64
#include "boot.h"

#include <string>
#include <sys/types.h>

#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
//#include <cunistd>
#include <cstdio> // for perror()
#include <cstdlib> // for atoi()


// Prototypes for transferring to C-style, if we choose to do so.
// VDIFile *vdiOpen(const char *fileName);
// void vdiClose(VDIFile *f);
// off_t vdiSeek(VDIFile *f, const off_t offset, const int anchor);
// size_t vdiRead(VDIFile *f, void *buf, const size_t count);
// size_t vdiWrite(VDIFile *f, const void *buf, const size_t count);


namespace vdi_explorer{
    class vdi_reader
    {
        public:
            vdi_reader(std::string fs);
            ~vdi_reader(void);
            
            off_t vdiSeek (off_t offset , int anchor );
            //Moves the file cursor to the specified place within the VDI file’s virtual disk.

            size_t vdiRead(size_t *buf , size_t count); // Deprecated.
            size_t vdiRead(void *buf, size_t count);
            //Reads count bytes from the file, placing them in the specified buffer.
            
            

            size_t vdiWrite(const void *buf , size_t count);
            //Writes count bytes to the file from the buffer.




            void vdiOpen(const std::string fileName);
            //Opens a VDI file and performs initialization.


            void vdiClose(void);
            //Closes a VDI file and performs necessary cleanup.

            

//            uint8_t *read_bytes(size_t block_number, size_t block_size);
        private:
            struct VDIHeader {
                char title[64];
                u32 magic;
                u16 majorVer, minorVer;
                u32 headerSize, imageType, imageFlags;
                char desc[256];
                u32 offsetPages, offsetData, nCylinders,
                    nHeads, nSectors, sectorSize, unused0;
                u64 diskSize;
                u32 pageSize, pageExtra, totalPages, pagesAllocated;
                u8 thisUUID[16], lastSnapUUID[16], linkUUID[16], parentUUID[16],
                    padding1[56]; 
            };
            
            // struct VDIFile {
            //     VDIHeader hdr;
            //     s32 fd;
            //     off_t cursor;
            //     s32 *pageMap;
            //     u8 *pageBitmap;
            //     u8 *dirtyBitmap;
            // };

            //VDIHeader hdr;
            //off_t translate(off_t);
            //int *pageMap;
            u8 *MBR = nullptr;
            BootSector bootSector;
            int *superBlock = nullptr;
            //int *bootSector = nullptr;
            //int fd;
            //off_t cursor;
            
            // Extracted from VDIFile struct.
            VDIHeader hdr;
            s32 fd;
            off_t cursor;
            s32 *pageMap = nullptr;
            u8 *pageBitmap = nullptr;
            u8 *dirtyBitmap = nullptr;
            
            off_t vdiTranslate(void);
            void vdiAllocatePageFrame(void);
            
            // @TODO Think about making a small function to do the calculation
            //       of cursor / hdr.pageSize utilizing bitshift, since it's a
            //       common function.
            
            

    };
}
#endif // VDI_READER_H