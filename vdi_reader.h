#ifndef VDI_READER_H
#define VDI_READER_H

#include "datatypes.h" //typedefs for s8, u8, s16, u16, s32, u32, s64, and u64

#include <string>
#include <sys/types.h>

namespace vdi_explorer{
    class vdi_reader
    {
        public:
            // Constructor
            vdi_reader(std::string fs);
            
            // Destructor
            ~vdi_reader();
            
            // Opens a VDI file and performs initialization.
            void vdiOpen(const std::string fileName);
            
            // Closes a VDI file and performs necessary cleanup.
            void vdiClose();
            
            // Moves the file cursor to the specified place within the VDI file's virtual disk.
            off_t vdiSeek(off_t offset, int anchor);
            
            // Reads count bytes from the file, placing them in the specified buffer.
            size_t vdiRead(void *buf, size_t count);
            
            // Writes count bytes to the file from the buffer.
            size_t vdiWrite(const void *buf, size_t count);
            
        private:
            struct __attribute__((packed)) VDIHeader {
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
            
            // Extracted from VDIFile struct.
            VDIHeader hdr;
            s32 fd;
            off_t cursor;
            s32 *pageMap = nullptr;
            u8 *pageBitmap = nullptr;
            u8 *dirtyBitmap = nullptr;

            // Performs the virtual-to-physical address translation.
            off_t vdiTranslate();
            
            // Allocates a new page frame in the VDI file.
            void vdiAllocatePageFrame();
            
            // @TODO Think about making a small function to do the calculation
            //       of cursor / hdr.pageSize utilizing bitshift, since it's a
            //       common function.
    };
} // namespace vdi_explorer

#endif // VDI_READER_H