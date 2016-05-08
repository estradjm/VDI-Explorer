#ifndef VDI_READER_H
#define VDI_READER_H

#include <string>
#include <sys/types.h>

typedef unsigned int u32;
typedef unsigned short int u16;
typedef unsigned long long int u64;

struct VDIHeader {
  char title[64];
  u32 magic;
  u16 majorVer,minorVer;
  u32 headerSize,imageType,imageFlags;
  char desc[256];
  u32 offsetPages,offsetData,nCylinders,
      nHeads,nSectors,sectorSize,unused0;
  u64 diskSize;
  u32 pageSize,pageExtra,totalPages,pagesAllocated;
};

namespace vdi_explorer{
    class vdi_reader
    {
        public:
            vdi_reader(std::string fs);
            ~vdi_reader(void);

            ssize_t read(void *,ssize_t);
            ssize_t write(const void *,ssize_t);

            off_t lseek(off_t,int);
//            void read_header();
//            uint8_t *read_bytes(size_t block_number, size_t block_size);
        //protected:
        private:
            VDIHeader hdr;
//            std::string fin;
            off_t translate(off_t);
            int *pageMap;
            int fd;
            off_t cursor;
    };
}
#endif // VDI_READER_H