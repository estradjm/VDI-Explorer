#include "constants.h"
#include "ext2.h"
#include "interface.h"
#include "utility.h"

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <string>
#include <stdexcept>
#include <stdio.h>

#define RESET		0
#define BRIGHT 		1
#define DIM		    2
#define UNDERLINE 	3
#define BLINK		4
#define REVERSE		7
#define HIDDEN		8

#define BLACK 		0
#define RED		    1
#define GREEN		2
#define YELLOW		3
#define BLUE		4
#define MAGENTA		5
#define CYAN		6
#define	WHITE		7

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
        vector<string> tokens2;

        while (true)
        {
            cout << "\nCommand: ";
            getline(cin, command_string);
            tokens = utility::tokenize(command_string, DELIMITER_SPACE);
            tokens2 = utility::tokenize(command_string, DELIMITER_FSLASH);
            
            // Debug info.
            for (unsigned int i = 0; i < tokens.size(); i++)
                cout << "debug (ext2::interface::interactive): " << tokens[i] << endl;
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
                
                // Debug
                case code_dump_pwd_inode:
                    command_dump_pwd_inode();
                    break;
                
                case code_dump_block:
                    command_dump_block(stoi(tokens[1]));
                    break;
                
                case code_dump_inode:
                    command_dump_inode(stoi(tokens[1]));
                    break;
                // End debug.
                
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
        file_system->set_pwd(directory);
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
    
    
    // @TODO format output neatly into appropriately sized columns -> function in utility?
    // @TODO sort vector by name
    // @TODO implement "a" and "l" switches
    // @TODO colorize: 
        // Blue: Directory
        // Green: Executable or recognized data file
        // Sky Blue: Linked file
        // Yellow with black background: Device
        // Pink: Graphic image file
        // Red: Archive file
    // @TODO add '/' to directories when displayed
    void interface::command_ls(const string & switches)
    {
        vector<fs_entry_posix> file_listing = file_system->get_directory_contents();
        vector<string> filename_tokens;
        for (u32 i = 0; i < file_listing.size(); i++)
        {
            filename_tokens = utility::tokenize(file_listing[i].name, DELIMITER_DOT);
            string file_extension = (filename_tokens.size() == true ? filename_tokens.back() : "");
            if (file_listing[i].type == EXT2_DIR_TYPE_DIR)
                cout << "\033[1;34m" << file_listing[i].name << "\033[0m"; //blue (bold) - directory or recognized data file
            else if (file_listing[i].type == EXT2_INODE_TYPE_FILE &&
                    (file_listing[i].permissions & EXT2_INODE_PERM_USER_EXECUTE ||
                     file_listing[i].permissions & EXT2_INODE_PERM_GROUP_EXECUTE ||
                     file_listing[i].permissions & EXT2_INODE_PERM_OTHER_EXECUTE))
                cout << "\033[2;31m" << file_listing[i].name << "\033[0m"; //green - executable files
            else if (file_listing[i].type == EXT2_INODE_TYPE_SYMLINK)
                cout << "\033[6;31m" << file_listing[i].name << "\033[0m"; //cyan - linked file
            else if (file_listing[i].type == EXT2_INODE_TYPE_SYMLINK)
                cout << "\033[3;31m" << file_listing[i].name << "\033[0m"; //yellow (with black background) - device
            else if (file_listing[i].type == EXT2_INODE_TYPE_SYMLINK)
                cout << "\033[5;31m" << file_listing[i].name << "\033[0m"; //pink - graphic image file
            else if (file_listing[i].type == EXT2_INODE_TYPE_FILE &&
                    (file_extension == "zip" ||
                     file_extension == "tar" ||
                     file_extension == "rar" ||
                     file_extension == "7z" ||
                     file_extension == "xz"))
                cout << "\033[0;31m" << file_listing[i].name << "\033[0m"; //red - archive file
            else 
                cout << file_listing[i].name;
            
            cout << "\t";
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
    
    
    // Debug.
    void interface::command_dump_pwd_inode()
    {
        file_system->debug_dump_pwd_inode();
    }
    void interface::command_dump_block(u32 block_to_dump)
    {
        file_system->debug_dump_block(block_to_dump);
    }
    void interface::command_dump_inode(u32 inode_to_dump)
    {
        file_system->debug_dump_inode(inode_to_dump);
    }
    // End debug.
    
    
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
                else if (command == "ls -l") // long list version of ls
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
        // Debug.
        else if (command == "dump_pwd_inode")
        {
            return code_dump_pwd_inode;
        }
        else if (command == "dump_block")
        {
            return code_dump_block;
        }
        else if (command == "dump_inode")
        {
            return code_dump_inode;
        }
        // End debug.
        else
        {
            return code_unknown;
        }
    }
} // namespace vdi_explorer