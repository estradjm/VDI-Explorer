# VDI_Explorer 
<p>Traversal and manipulation of an EXT2 Filesystem contained in VirtualBox .VDI File, without mounting in a virtual machine.</p>

##Description:
<p>Program interface is command line based. The program gives the user the ability to traverse an EXT2 filesystem contained within a VirtualBox .VDI file. The program can display all files and content inside the .VDI file. The program can also write write files to and from the host machine to the the filesystem contained within the .VDI file, without corruption and without the need of mounting the .VDI file in a virtual machine. </p>

<p>The user uses the program essentially like a linux terminal with the following commands:</p>
<ul>
  <li>`ls -l`  Create a listing of files and directory in current working directory</li>
  <li>`cd {directory}` Change directory</li>
  <li>`cp {source} {destination}` Copy a file between VDI and host</li>
  <li>`exit` Exit out of VDI file traversal program</li>
</ul>

##Installation:
<p>To compile the project, type in the following commands: </p>
<ul>
  <li>`make`</li>
</ul>

<p>To remove all object files and executables:</p>
<ul>
  <li>`make clean`</li>
</ul>

##Example:
<p>To run, depending on which file you want to give in as the VDI input file:</p>
<ul>
  <li>`./vdi Test-dynamic-1k.vdi`</li>
  <li>`./vdi Test-fixed-4k.vdi`</li>
  <li>`./vdi Test-fixed-1k.vdi`</li>
</ul>

##License: 
Apache ver. 2.0, Copyright Jenniffer Estrada, 2016
