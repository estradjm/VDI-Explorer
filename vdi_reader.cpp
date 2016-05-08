#include "vdi_reader.h"
//#include <fstream>
#include <iostream>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

/* VDI reader should read the VDI header and populate the VDI header structure, and read the VDI Map into pageMap*/

using namespace std;

typedef unsigned char uint8_t;

namespace vdi_explorer{

vdi_reader::vdi_reader(string fs){

  fd = open(fs.c_str(),O_RDWR);

  ::read(fd,&hdr,sizeof(hdr));

  cout << "VDI Header Information:" << endl;
  cout << "Offset: " << hdr.offsetData << endl; //2097152
  cout << "Magic Number: " << hdr.magic << endl; 
  cout << "Header Size: " << hdr.headerSize << endl; 
  cout << "Offset Pages: " << hdr.offsetPages << endl; 
  cout << "Disk Size: " << hdr.diskSize << endl; 
  cout << "Sector Size: " << hdr.sectorSize<< endl; 
  cout << "Page Size: " << hdr.pageSize<< endl; 
  cout << "Page Extra: " << hdr.pageExtra<< endl; 
  cout << "Total Pages: " << hdr.totalPages<< endl; 
  cout << "Pages Allocated: " << hdr.pagesAllocated<< endl; 

  ::lseek(fd,hdr.offsetPages,SEEK_SET);

  pageMap = new int[hdr.totalPages]; //totalPages is the same as offsetBlocks? Value is 128

  ::read(fd,pageMap,hdr.totalPages*sizeof(int)); // totalPages is the same as nBlocks? (128 * 4?)
  
  cout << "VDI Map: " << pageMap<< endl; // this address corresponds to the start of VDI Map?

}

vdi_reader::~vdi_reader(){
    ::close(fd);
}

off_t lseek(off_t offset,int ){
     //off_t  = translate(off_t )

}


/* vdi_reader::read_header(){
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