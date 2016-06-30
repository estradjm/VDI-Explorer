# VDI_Explorer 
<p>Traversal and manipulation of an EXT2 Filesystem contained in VirtualBox .VDI File, without mounting in a virtual machine.</p>

##Description:
<p>Program interface is command line based. The program gives the user the ability to traverse an EXT2 filesystem contained within a VirtualBox .VDI file. The program can display all files and content inside the .VDI file. The program can also write write files to and from the host machine to the the filesystem contained within the .VDI file, without corruption and without the need of mounting the .VDI file in a virtual machine. </p>

<p>The user uses the program essentially like a linux terminal with the following commands:</p>

  * `ls -l`  Create a listing of files and directory in current working directory
  * `cd {directory}` Change directory
  * `cp {source} {destination}` Copy a file between VDI and host
  * `exit` Exit out of VDI file traversal program

##Installation:
<p>To compile the project, type in the following commands: </p>
  *`make`

<p>To remove all object files and executables:</p>
  *`make clean`

##Example:
<p>To run, depending on which file you want to give in as the VDI input file:</p>
  * `./vdi Test-dynamic-1k.vdi`</li>
  * `./vdi Test-fixed-4k.vdi`</li>
  * `./vdi Test-fixed-1k.vdi`</li>


##License: 
Apache ver. 2.0, Copyright Jenniffer Estrada, 2016
