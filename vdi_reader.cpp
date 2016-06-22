#include "vdi_reader.h"
#include "datatypes.h"
#include <iostream>
#include <iomanip>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>

#define DEBUG_WRITE_DISABLED

/* VDI reader should read the VDI header and populate the VDI header structure, and read the VDI Map into pageMap*/

using namespace std;

typedef unsigned char uint8_t;

namespace vdi_explorer{
  
    //enum anchor{SEEK_END = hdr.diskSize, SEEK_CUR = cursor, SEEK_SET=0};

vdi_reader::vdi_reader(string fs){
  vdiOpen(fs);
  // return;
  
  // fd = open(fs.c_str(),O_RDWR);
  // ::lseek(fd, 0, 0 );

  // ::read(fd,&hdr,sizeof(hdr));

  cout << "VDI Header Information:" << endl;
  cout << "Dynamic/Static: " << (hdr.imageType == 1 ? "1 - dynamic" : "2 - static") << endl; //2 - static, 1 - dynamic
  cout << "MBR Start Offset: " << hdr.offsetData << endl; //2097152 - Where MBR is located
  cout << "Magic Number: " << hdr.magic << endl; // Image Signature
  cout << "Header Size: " << hdr.headerSize << endl; 
  cout << "Offset Pages: " << hdr.offsetPages << endl; 
  cout << "Disk Size: " << hdr.diskSize << endl; 
  cout << "Sector Size: " << hdr.sectorSize<< endl; 
  cout << "Page Size: " << hdr.pageSize<< endl; 
  cout << "Page Extra: " << hdr.pageExtra<< endl; 
  cout << "Total Pages: " << hdr.totalPages<< endl; // offsetBlocks and blocksInHDD
  cout << "Pages Allocated: " << hdr.pagesAllocated<< endl; 

  // ::lseek(fd,hdr.offsetPages,SEEK_SET);

  // pageMap = new int[hdr.totalPages]; //totalPages is the same as offsetBlocks? Value is 128

  // ::read(fd,pageMap,hdr.totalPages*sizeof(int)); // totalPages is the same as nBlocks? (128 * 4 = 512)
  
  //try to get MBR (end of MBR = 0xaa55 = 43605)
  //vdiSeek(hdr.offsetData,0);
  
  // ::lseek(fd,3788993 - hdr.sectorSize,SEEK_SET); //::lseek(fd,hdr.offsetData,SEEK_CUR);

  MBR = new u8[hdr.sectorSize];
  vdiSeek(0, SEEK_SET);
  vdiRead(MBR, hdr.sectorSize); 

  
  vdiSeek(0, SEEK_SET);
  vdiRead(&bootSector, hdr.sectorSize); 
  // ::read(fd,MBR,hdr.sectorSize); 
  
//  cout << "MBR: " << MBR<< endl; 
  
  cout << "\nMBR: ";
  for (int i=0; i < hdr.sectorSize; i++){
    cout << hex << setfill('0') << setw(2) << (int)MBR[i] << " ";
  }
  cout << endl << endl;

  cout << "\nBootSector struct: ";
  cout << "\nBootstrap code: ";
  for (int i = 0; i < 0x1be; i++)
    cout << hex << setfill('0') << setw(2) << (int)bootSector.unused0[i] << dec << " ";
  for (int i = 0; i < 4 /*number of partition tables*/; i++) {
    cout << "\nPartition Entry " << i+1 << ": ";
    cout << "\n  First Sector: " << bootSector.partitionTable[i].firstSector;
    cout << "\n  nSectors: " << bootSector.partitionTable[i].nSectors;
  }
  cout << "\nBoot Sector Magic Number: " << hex << bootSector.magic << dec << endl; //BOOT_SECTOR_MAGIC

  cout << "\nVDI Map: ";
  for (int i=0; i < hdr.totalPages; i++){
    cout << dec << pageMap[i] << " ";
  }
  cout << endl << endl;
  


}

vdi_reader::~vdi_reader(){
    //::close(fd);
    vdiClose();
    if (MBR)
      delete[] MBR;
    if (superBlock)
      delete[] superBlock;
    // if (bootSector)
    //   delete[] bootSector;
}

off_t vdi_reader::vdiSeek (off_t offset , int anchor){
  //Moves the file cursor to the specified place within the VDI file’s virtual disk
    /*The anchor is a reference point. vdiSeek sets the cursor to 
  anchorPoint + offset, where anchorPoint is 0 for SEEK_SET, 
  the current value of cursor for SEEK_CUR and the disk size 
  (in the VDI header) for SEEK_END.*/
  off_t location;
  if (anchor == SEEK_SET)
    location = offset;
  else if (anchor = SEEK_CUR)
    location = cursor + offset;
  else 
    location = hdr. diskSize + offset;
  if (location < 0 || location >= hdr.diskSize)
    location = -1;
  else
    cursor = location;
  return location;
    
  //cursor = hdr.offsetPages + anchor + offset;
  //return location;
}

// Deprecated. Move to using vdiRead(void *buf, size_t count).
/*
size_t vdi_reader::vdiRead(size_t *buf , size_t count){
  //Reads count bytes from the file, placing them in the specified buffer.
  size_t buffer[count];
  for (int i=0;i<count; i++){
    vdiSeek( i, 0);
    * (buffer + i)=*buf + cursor + i;
  }
  return * buffer;
}
*/
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
  
  return nBytes;
}


size_t vdi_reader::vdiWrite(const void *buf , size_t count){
  // //Writes count bytes to the file from the buffer. NEEDS MODIFICATION!!!
  // size_t buffer[count];
  // for (int i=0;i<count; i++){
  //   vdiSeek(i, 0);
    
  //   //* (buffer + i)=*buf + cursor + i;
  // }
  // //return * buffer;
  
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
        // If the specified location has still not been allocated, A Bad Thing
        // has happened.  Break the loop.
        break;
      }
    }
    #ifndef DEBUG_WRITE_DISABLED
    ::lseek(fd, location, SEEK_SET);
    ::write(fd, ((u8 *)buf) + nBytes, chunkSize);
    #endif
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
  
  return nBytes;
}

// @TODO Magic numbers >> consts
// @TODO More comments.
// @TODO Make actual exceptions, not simply generic ones.
void vdi_reader::vdiOpen(const std::string fileName)
{
  // Open the file and read the header.
  //
  // Once the VDIFile structure has been allocated, a field is available to hold
  // the file descriptor. An attempt is made to open the file; if successful,
  // the VDI header is loaded.
  //
  // If the open fails for any reason or if the VDI header has the wrong magic
  // number, then the VDIFile structure is deallocated and NULL is returned.
  fd = ::open(fileName.c_str(), O_RDWR);
  if (fd == -1)
  {
    cout << "Error opening file.\n";
    throw;
  }
  size_t nBytes = read(fd, &hdr, sizeof(VDIHeader));
  if (nBytes != sizeof(VDIHeader) || hdr.magic != 0xbeda107f)
  {
    cout << "Unexpected header size or the magic number has lost its magic.\n";
    throw;
  }
  
  // Allocate the page map and bitmaps.
  // 
  // The page map is allocated dynamically, since the size isn’t known until the
  // VDI header is loaded.
  //
  // Since the page map may be quite large, it is loaded on an as-needed basis.
  // The map is divided into 4KB chunks with each chunk holding 4096/4 = 1024
  // map entries. A chunk is loaded into the map the first time any entry within
  // the chunk is needed.
  //
  // A bitmap is allocated to keep track of which chunks have been loaded. A
  // second bitmap is allocated to keep track of which chunks have been
  // modified. A chunk can be modified if one of its entries changes, which
  // occurs when a previously unallocated page is allocated.
  pageMap = new s32[hdr.totalPages];
  if (pageMap == nullptr)
  {
    ::close(fd);
    cout << "Error allocating pageMap.\n";
    throw;
  }
  u32 bitmapSize = (hdr.totalPages + 8191)/8192;
  pageBitmap = new u8(bitmapSize);
  if (pageBitmap == nullptr)
  {
    ::close(fd);
    delete[] pageMap;
    pageMap = nullptr;
    cout << "Error allocating pageBitmap.\n";
    throw;
  }
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
  // All that remains in vdiOpen is to make sure that the fields are allocated
  // properly initialized. At this point, the file has been opened and the
  // header has been loaded, so fd and hdr are initialized. The page map does
  // not need to be initialized; entries will not be seen until their chunk has
  // been loaded. The page and dirty bitmaps must be cleared to indicate that no
  // chunks have been loaded or modified. Finally, the cursor must be set to 0.
  ::memset(pageBitmap, 0, bitmapSize);
  ::memset(dirtyBitmap, 0, bitmapSize);
  cursor = 0;
}

// @TODO Turn the magic numbers into constants.
void vdi_reader::vdiClose(void)
{
  // Keep track of whether the header needs to be written.
  bool mustWriteHeader = false;
  
  // Determine the size of the dirty bitmap.
  u32 bitmapSize = (hdr.totalPages + 8191) / 8192;
  size_t chunkSize;
  
  // Scan through dirtyBitmap and write any modified chunks of the page map to
  // disk.
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
      
      // Write the updated page map to the VDI and set the flag so the header
      // will be written as well.
      #ifndef DEBUG_WRITE_DISABLED
      ::lseek(fd, hdr.offsetPages + i * 4096, SEEK_SET);
      ::write(fd, pageMap + i * 1024, chunkSize);
      #endif
      mustWriteHeader = true;
    }
  }
  
  // If the header needs to be written to disk, do it.
  if (mustWriteHeader)
  {
    #ifndef DEBUG_WRITE_DISABLED
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

// @TODO Yet again, turn the magic numbers into well-named constants.
off_t vdi_reader::vdiTranslate(void)
{
  u32 pageNum;
  off_t offset;
  
  // Check to make sure the cursor is pointing to somewhere valid on the vitual
  // disk.
  if (cursor >= hdr.diskSize)
  {
    return 0;
  }
  
  // Compute the page number and offset.
  pageNum = cursor / hdr.pageSize;
  offset = cursor - pageNum * hdr.pageSize;
  // @TODO Verify that these operations are accurate. If they are, replace the 2 lines above.
  //pageNum = cursor >> 20;
  //offset = cursor & 0xfffff;
  
  // Load page map chunk if necessary.
  u32 chunkNum = pageNum / 1024;
  // Check if the page map chunk is loaded.
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
  
  return offset;
}

void vdi_reader::vdiAllocatePageFrame(void)
{
  // Add page frame.
  u8 *tmpBuffer = new u8[hdr.pageSize];
  ::memset(tmpBuffer, 0, hdr.pageSize);
  ::lseek(fd, 0, SEEK_END);
  ::write(fd, tmpBuffer, hdr.pageSize);
  delete[] tmpBuffer;
  
  // Update the page map.
  u32 pageNum = cursor / hdr.pageSize;
  pageMap[pageNum] = hdr.pagesAllocated;
  hdr.pagesAllocated++;
  u32 chunkNum = pageNum / 1024;
  dirtyBitmap[chunkNum / 8] |= (1 << (chunkNum % 8));
}

/*off_t lseek(off_t offset,int ){
     //off_t  = translate(off_t )

}


vdi_reader::read_header(){
ifstream is(fin.c_str());
  if (is) {
    // get length of file:
    // 1/2 k header in the beginning of .vdi file
    is.seekg (0, is.end);
    int length = is.tellg();
    is.seekg (0, is.beg);
    //int length = 4;


    uint8_t * buffer = new uint8_t [length];

    cout << "Reading " << length << " characters... \n";
    // read data as a block:
    is.read ((char *)buffer,length);


    for (int i = 0 ; i < 400; i++){
        if (i % 16 == 0) cout << endl << i << ": ";
        cout << std::hex << (unsigned int) buffer[i] << " ";
    }

    if (is)
      cout << "\n\nall characters read successfully.";
    else
      cout << "error: only " << is.gcount() << " could be read";


    //is.seekg(0x177, is.beg);
    //is.read((char*)buffer, 4);

    int block_size =  buffer[0x177] + 256 * (buffer[0x178] + 256 * (buffer[0x179] + 256 * buffer[0x17a]));

    cout << "\nBlock size: " << std::hex << block_size << endl;

    is.close();

    delete[] buffer;
  }
}
*/
}

