# VDI_Explorer: 
Traversal and manipulation of an EXT2 Filesystem contained in VirtualBox .VDI File, without mounting in a virtual machine.

To compile the project, type in the following commands: 
$ make vdi

To run, depending on which file you want to give in as the VDI input file:
$ ./vdi Test-dynamic-1k.vdi
$ ./vdi Test-fixed-4k.vdi
$ ./vdi Test-fixed-1k.vdi

Interface with the program is command line based and interactive, giving the user the ability to traverse a VirtualBox .VDI file, read files from inside the binary, and write files to the binary without corruption, without the need of mounting the .VDI file in a virtual machine. 

The user uses the program essentially like a linux terminal with the following commands:
ls -l ==> create a listing of files and directory in current working directory
cd <directory> ==> change directory
cp <source> <destination> ==> copy a file between VDI and host
exit ==> exit out of VDI file traversal program
