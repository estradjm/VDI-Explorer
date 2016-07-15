#include "constants.h"
#include "ext2.h"
#include "interface.h"
#include "utility.h"

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <string>
#include <stdexcept>

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
        string command_string;
        vector<string> tokens;

        while (true)
        {
            cout << "\nCommand: ";
            getline(cin, command_string);
            tokens = utility::tokenize(command_string, DELIMITER_SPACE);
            
            // Debug info.
            for (unsigned int i = 0; i < tokens.size(); i++)
                cout << tokens[i] << endl;
            // End debug info.
            
            if (tokens.size() == 0)
                continue;
                
            switch (hash_command(tokens[0]))
            {
                case code_cd:
                    if (tokens.size() < 2)
                    {
                        cout << "Not enough arguments.\n";
                        command_help("cd");
                    }
                    else
                    {
                        command_cd(tokens[1]);
                    }
                    break;
                    
                case code_cp:
                    if (tokens.size() < 4)
                    {
                        cout << "Not enough arguments.\n";
                        command_help("cp");
                    }
                    else
                    {
                        command_cp(tokens[1], tokens[2], tokens[3]);
                    }
                    break;
                    
                case code_exit:
                    command_exit();
                    break;
                    
                case code_help:
                    if (tokens.size() < 2)
                    {
                        command_help("");
                    }
                    else
                    {
                        command_help(tokens[1]);
                    }
                    break;
                    
                case code_ls:
                    if (tokens.size() < 2)
                    {
                        command_ls("");
                    }
                    else
                    {
                        command_ls(tokens[1]);
                    }
                    break;
                    
                case code_pwd:
                    command_pwd();
                    break;
                
                case code_unknown:
                    cout << "Unknown command.\n";
                    break;
                    
                default:
                    // A Bad Thing happened.  This should never get triggered.
                    cout << "A Bad Thing happened.  You should not be seeing this.";
                    break;
            }
        }
    }
    
    void interface::command_cd(const string & directory)
    {
        // stub
        cout << "Not implemented yet.\n";
        return;
    }
    
    void interface::command_cp(const string & direction,
                               const string & copy_from,
                               const string & copy_to)
    {
        // stub
        cout << "Not implemented yet.\n";
        return;
    }
    
    // @TODO format output neatly into appropriately sized columns
    // @TODO sort vector by name
    // @TODO implement "-al" switch
    // @TODO colorize?
    // @TODO add '/' to directories when displayed
    void interface::command_ls(const string & switches)
    {
        vector<fs_entry_posix> file_listing = file_system->list_directory_contents();
        for (u32 i = 0; i < file_listing.size(); i++)
        {
            cout << file_listing[i].name << "\t";
        }
        return;
    }
    
    void interface::command_exit()
    {
        // exit the program
        exit(0);
    }
    
    void interface::command_help(const string & command)
    {
        command_code hashed_command = hash_command(command);
        switch (hashed_command)
        {
            case code_none:
            case code_cd:
                // explain cd command
                cout << "cd <directory_to_change_to>\n";
                cout << "Change directory command. This command will change the present working " <<
                        "directory to the one specified.\n";
                if (hashed_command != code_none)
                    break;
                else
                    cout << endl;
                
            case code_cp:
                // explain cp command
                cout << "cp <in|out> <file_to_copy_from> <file_to_copy_to>\n";
                cout << "Copy a file between the host OS and the virtual hard drive and vice " <<
                        "versa.\n";
                if (hashed_command != code_none)
                    break;
                else
                    cout << endl;
                
            case code_exit:
                // explain exit command
                cout << "exit\n";
                cout << "Exits the program.\n";
                if (hashed_command != code_none)
                    break;
                else
                    cout << endl;
                
            case code_help:
                // explain help command
                cout << "help [command]\n";
                cout << "Procides help on the various commands.\n";
                if (hashed_command != code_none)
                    break;
                else
                    cout << endl;
                
            case code_ls:
                // explain ls command
                cout << "ls [-al]\n";
                cout << "List the contents of the present working directory.\n";
                if (hashed_command != code_none)
                    break;
                else
                    cout << endl;
                
            case code_pwd:
                // explain pwd command
                cout << "pwd\n";
                cout << "Prints out the present working directory.\n";
                break;

            case code_unknown:
                // unknown command
                cout << command << endl;
                cout << "Unknown command.\n";
                break;
            
            default:
                // A Bad Thing happened.  This should never get triggered.
                cout << "A Bad Thing happened.  You should not be seeing this.";
                break;
        }
    }
    
    void interface::command_pwd()
    {
        cout << file_system->get_pwd() << endl;
        return;
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
        else if (command == "")
        {
            return code_none;
        }
        else
        {
            return code_unknown;
        }
    }
} // namespace vdi_explorer