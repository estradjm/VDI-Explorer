# VDI_Explorer
Traversal and manipulation of an EXT2 Filesystem contained in VirtualBox VDI File

To run the project, type in the following commands: 
$ make vdi

Depending on which file you want to give in as the VDI input file:
$ ./vdi Test-dynamic-1k.vdi
$ ./vdi Test-fixed-4k.vdi
$ ./vdi Test-fixed-1k.vdi

Figured the user could use the program essentially like a linuxx terminal with the following commands:
ls -l ==> create a listing of files and directory in current working directory - not implemented
exit ==> exit out of terminal 
cd <directory> ==> change directory - not implemented
cp <source> <destination> ==> copy a file between VDI and host - not implemented