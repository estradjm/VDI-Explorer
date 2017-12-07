//VDI Layer:

VDIFile *vdiOpen(const char *filename)
//Opens a VDI file, performs initialization and returns a pointer to a VDIFile data structure.


void vdiClose(VDIFile * ƒ )
//Closes a VDI file and performs necessary cleanup.


off t vdiSeek (VDIFile * ƒ , off t offset , int anchor )
//Moves the file cursor to the specified place within the VDI file’s virtual disk.



size t vdiRead(VDIFile * ƒ , void *buf , size t count)
//Reads count bytes from the file, placing them in the specified buffer.


size t vdiWrite(VDIFile * ƒ , const void *buf , size t count)
//Writes count bytes to the file from the buffer.


/*
-> The VDI header, which holds key information about the virtual disk as well as other information
that is important but not relevant to this project;

-> The page map, which holds the page frame number of each page of the virtual disk;

-> The virtual disk, which is divided into equal-sized pages. The pages do not have to appear in
order, nor do all pages have to be allocated.

The landmarks headerSize, offsetPageMap and offsetData are all defined in the VDI header.

The VDI header is a simple structure; it is described in the next section. The page map is an array
of signed 32-bit integers, one entry for each page of the virtual disk.

*/





// seek.cc
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <cunistd>
#include <cstdio> // for perror()
#include <cstdlib> // for atoi()

using namespace std;

char
  line[128];

int main(int argc,char *argv[]) {
  int fd,nBytes;
  off_t offset,whereami;

  fd = open(argv[1],O_RDONLY);
  if (fd == -1) {
    perror("open");
    return 1;
  }

  offset = atoi(argv[2]);

  whereami = lseek(fd,offset,SEEK_SET);
  cout << "Now at position: " << whereami << endl;

  while ((nBytes = read(fd,line,sizeof(line))) != 0) {
    write(1,line,nBytes);
  }

  close(fd);
}





//write.cc
#include <iostream>
//---
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
// sometimes #include <cunistd>
#include <cstdio> // for perror()

using namespace std;

char
  line[128];

int main(int argc,char *argv[]) {
  int fd,nBytes;

//  fd = open(argv[1],O_WRONLY,0666); // file must exist
//  fd = open(argv[1],O_WRONLY|O_CREAT,0666); // create if necessary
//  fd = open(argv[1],O_WRONLY|O_CREAT|O_EXCL,0666); // must create, fail if exists
//  fd = open(argv[1],O_WRONLY|O_CREAT|O_TRUNC,0666); // wipe file on open
  fd = open(argv[1],O_WRONLY|O_CREAT|O_APPEND,0666); // keep data, append new data
  if (fd == -1) {
    perror("open");
    return 1;
  }

  while ((nBytes = read(0,line,sizeof(line))) != 0) {
    write(fd,line,nBytes);
  }

  close(fd);
}






//read.cc
#include <iostream>
//---
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
// sometimes #include <cunistd>
#include <cstdio> // for perror()

using namespace std;

char
  line[128];

int main(int argc,char *argv[]) {
  int fd,nBytes;

  fd = open(argv[1],O_RDONLY);
  if (fd == -1) {
    perror("open");
    return 1;
  }

  while ((nBytes = read(fd,line,sizeof(line))) != 0) {
    write(1,line,nBytes);
  }

  close(fd);
}

