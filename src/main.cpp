/*--------------------------------------------------------------------------------------------------
 * Author:      
 * Date:        2016-08-18
 * Assignment:  Final Project
 * Source File: main.cpp
 * Language:    C/C++
 * Course:      Operating Systems
 * Purpose:     Contains the implementation of main.
 -------------------------------------------------------------------------------------------------*/
 
#include <iostream>
#include "ext2.h"
#include "interface.h"
#include "utility.h"
#include "vdi_reader.h"
#include <vector>
#include <string>

using namespace std;

int main(int argc, char *argv[])
{
    string cmd, filename;
    // bool flag;
    
    // grab VDI file passed into program
    if (argc > 1){
        //cout << argc <<" " << argv[0]<< " " << argv[1]<< " " << argv[2] << endl;
        filename = argv[1];
        cout << "Reading file: " << argv[1] << "\n"<< endl;
        vdi_explorer::vdi_reader fs(filename);
        vdi_explorer::ext2 e2(&fs);
        
        // Debug info.
        // string temp_string; temp_string.assign("      this ||is   a test  ||   ");
        // string temp_delim; temp_delim.assign(" ");
        // vector<string> temp_tokens = utility::tokenize(temp_string, temp_delim);
        // for (u32 i = 0; i < temp_tokens.size(); i++)
        //     cout << "'" << temp_tokens[i] << "'" << endl;
        // End debug info.

        vdi_explorer::interface my_interface(&e2);
        my_interface.interactive();
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
