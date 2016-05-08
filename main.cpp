
#include <iostream>
#include "vdi_reader.h"

using namespace std;

int main(int argc, char *argv[])
{
string cmd, filename;
bool flag;

// grab VDI file passed into program
if (argc > 1){
    //cout << argc <<" " << argv[0]<< " " << argv[1]<< " " << argv[2] << endl;
    filename = argv[1];
    cout << "Reading file: " << argv[1] << "\n"<< endl;
    vdi_explorer::vdi_reader fs(filename);
}
//vdi_explorer::vdi_reader fs(filename);

/*
do{


cin >> cmd;



// Exit out of Program when exit command is inputed
cmd == "exit" ? flag = 0 : flag = 1;

}
while (flag);
*/


    return 0;
}
