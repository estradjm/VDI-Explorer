#include "vdi_reader.h"
//#include <fstream>
#include <iostream>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

/* VDI reader should read the VDI header and populate the VDI header structure*/

using namespace std;

typedef unsigned char uint8_t;

namespace vdi_explorer{

vdi_reader::vdi_reader(string fs){

  fd = open(fs.c_str(),O_RDWR);

  ::read(fd,&hdr,sizeof(hdr));

  cout << hdr.offsetData << endl; //2097152

  ::lseek(fd,hdr.offsetPages,SEEK_SET);

  pageMap = new int[hdr.totalPages];

  ::read(fd,pageMap,hdr.totalPages*sizeof(int));

}

vdi_reader::~vdi_reader(){
    ::close(fd);
}

off_t lseek(off_t offset,int ){
     //off_t  = translate(off_t )

}


/*oid vdi_reader::read_header(){
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
