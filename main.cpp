/*
Commands:
ls -l - create a listings - not implemented
exit - exit out of terminal - works
cd - change directory - not implemented
cp <source> <destination> - copy a file between VDI and host - not implemented
*/

#include <iostream>
#include "vdi_reader.h"

using namespace std;

int main(int argc, char *argv[])
{
string cmd, filename;
bool flag;
cout << "VDI Explorer: " << endl;

// grab file passed into program or use default with IDE
if (argc > 1)
    filename = argv[argc];
else
    filename = "/home/jenniffer/Desktop/Operating Systems/Test-fixed-1k.vdi";

vdi_explorer::vdi_reader fs(filename);


do{


cin >> cmd;



// Exit out of Program when exit command is inputed
cmd == "exit" ? flag = 0 : flag = 1;

}
while (flag);



    return 0;
}
