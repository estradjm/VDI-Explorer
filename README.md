# <b> VDI Explorer</b> : <em>Break Out Of The BOX</em>
<p>Traversal and manipulation of an EXT2 Filesystem contained in VirtualBox .VDI File, without mounting in a virtual machine.</p>

## Description:
<p>Program interface is command line based. The program gives the user the ability to traverse an EXT2 filesystem contained within a VirtualBox .VDI file. The program can display all files and content inside the .VDI file. The program can also write files into and out of the filesystem contained within the .VDI file to the host machine, without corruption and without the need of mounting the .VDI file in a virtual machine. </p>

<p>The user uses the program essentially like a linux terminal with the following commands:</p>

  * `ls [-al]`  Create a listing of files and directory in current working directory, adding -l for the long list, and -al for long list with all hidden files shown as well.
  * `pwd` Prints working directory.
  * `help {command}` Prints out command, usage, and what the command does. If no command specified, prints out all possible command information.
  * `cd {directory}` Change directory.
  * `cp [in|out] {file_to_copy_from} {file_to_copy_to}` Copy a file between VDI and host.
  * `exit` Exit out of VDI file traversal program.

## Installation:
<p>To compile the project, type in the following commands: </p>
  * `make`

<p>To remove all object files and executables:</p>
  * `make clean`

## Example:
<p>To run, depending on which file you want to give in as the VDI input file:</p>
  * `./vdi Test-dynamic-1k.vdi`</li>
  * `./vdi Test-fixed-4k.vdi`</li>
  * `./vdi Test-fixed-1k.vdi`</li>

## License: 
Apache ver. 2.0, Copyright Jenniffer Estrada, 2016
