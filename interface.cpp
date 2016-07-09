#include "ext2.h"
#include "interface.h"
#include "utility.h"

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <string>

using namespace std;
using namespace vdi_explorer;

namespace vdi_explorer
{
    // @TODO Make an actual throwable error.
    interface::interface(ext2 * _file_system)
    {
        file_system = _file_system;
        if (file_system == nullptr)
        {
            cout << "Error opening the file system object.";
            throw;
        }
    }
    
    interface::~interface()
    {
        return;
    }
    
    void interface::interactive()
    {
        return;
    }
    
    bool interface::command_cd(const string & directory)
    {
        // stub
        return false;
    }
    
    bool interface::command_cp(const string & copy_from, const string & copy_to)
    {
        // stub
        return false;
    }
    
    bool interface::command_ls(const string & switches)
    {
        // stub
        return false;
    }
    
    void interface::command_exit()
    {
        // exit the program
        exit(0);
    }
    
    bool interface::command_help(const string & command)
    {
        switch (hash_command(command))
        {
            case code_cd:
                // explain cd command
                cout << "Command:       cd\n";
                cout << "Arguments:     <directory to change to>\n";
                cout << "Information:   Change directory command.  This command will change the\n";
                cout << "               present working directory to the one specified.\n";
                break;
                
            case code_cp:
                // explain cp command
                cout << "Command:       cp\n";
                cout << "Arguments:     <file to copy from> <file to copy to>";
                cout << "Information:   \n";
                break;
                
            case code_exit:
                // explain exit command
                cout << "Command:       exit\n";
                cout << "Arguments:     (none)\n";
                cout << "Information:   Exits the program.\n";
                break;
                
            case code_help:
                // explain help command
                cout << "Command:       \n";
                cout << "Arguments:     \n";
                cout << "Information:   \n";
                break;
                
            case code_ls:
                // explain ls command
                cout << "Command:       \n";
                cout << "Arguments:     \n";
                cout << "Information:   \n";
                break;
                
            case code_pwd:
                // explain pwd command
                cout << "Command:       pwd\n";
                cout << "Arguments:     (none)\n";
                cout << "Information:   Prints out the present working directory.\n";
                break;
                
            case code_unknown:
                // unknown command
                cout << "Command:       (unknown)\n";
                cout << "Arguments:     (unknown)\n";
                cout << "Information:   Unknown command.\n";
                break;
                
            default:
                // A Bad Thing happened.  This should never get triggered.
                cout << "A Bad Thing happened.  You should not be seeing this.";
                return false;
        }
        
        return true;
    }
    
    bool interface::command_pwd()
    {
        // stub
        return false;
    }
    
    interface::command_code interface::hash_command(const string & command)
    {
        if (command == "cd")
        {
            return code_cd;
        }
        else if (command == "cp")
        {
            return code_cp;
        }
        else if (command == "exit")
        {
            return code_exit;
        }
        else if (command == "help")
        {
            return code_help;
        }
        else if (command == "ls")
        {
            return code_ls;
        }
        else if (command == "pwd")
        {
            return code_pwd;
        }
        else
        {
            return code_unknown;
        }
    }
} // namespace vdi_explorer