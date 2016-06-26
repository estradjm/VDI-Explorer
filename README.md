# VDI_Explorer: 
<p>Traversal and manipulation of an EXT2 Filesystem contained in VirtualBox .VDI File, without mounting in a virtual machine.</p>

<p>To compile the project, type in the following commands: </p>
<ul>
  <li>$ make</li>
</ul>

<p>To remove all object files and executables:</p>
<ul>
  <li>$ make clean</li>
</ul>

<p>To run, depending on which file you want to give in as the VDI input file:</p>
<ul>
  <li>$ ./vdi Test-dynamic-1k.vdi</li>
  <li>$ ./vdi Test-fixed-4k.vdi</li>
  <li>$ ./vdi Test-fixed-1k.vdi</li>
</ul>

<p>Interface with the program is command line based and interactive, giving the user the ability to traverse a VirtualBox .VDI file, read files from inside the binary, and write files to the binary without corruption, without the need of mounting the .VDI file in a virtual machine. </p>

<p>The user uses the program essentially like a linux terminal with the following commands:</p>
<ul>
  <li>ls -l ==> create a listing of files and directory in current working directory</li>
  <li>cd {directory} ==> change directory</li>
  <li>cp {source} {destination} ==> copy a file between VDI and host</li>
  <li>exit ==> exit out of VDI file traversal program</li>
</ul>
