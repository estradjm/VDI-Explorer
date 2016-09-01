/*--------------------------------------------------------------------------------------------------
 * Author:      
 * Date:        2016-08-18
 * Assignment:  Final Project
 * Source File: interface.cpp
 * Language:    C/C++
 * Course:      Operating Systems
 * Purpose:     Contains the implementation of the interface class.
 -------------------------------------------------------------------------------------------------*/

#include "constants.h"
#include "ext2.h"
#include "interface.h"
#include "utility.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <stdexcept>
#include <cstdio>
#include <time.h>

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
        cout << "*** Implementation in progress.  Bust out the bugspray. ***\n";
        
        fstream os_file;
        
        if (direction == "in")
        {
            // cout << "Implementing... Please stand by.\n";
            // return;
            
            // Attempt to open the file for input to check if it exists.
            os_file.open(copy_from, ios_base::in | ios::binary);
            
            // Actually check if the file exists.
            if (!os_file.good())
            {
                // File does not exist, so display an error.
                cout << "Error: File does not exist.  (interface::command_cp)\n";
            }
            else
            {
                // File exists, so write it to the virtual disk.
                file_system->file_write(os_file, copy_to);
            }
            
            // Close the file.
            os_file.close();
            
            // Return.  Duh.
            return;
        }
        else if (direction == "out")
        {
            // Attempt to open the file for input to see if it already exists.
            os_file.open(copy_to, ios_base::in);
            bool file_exists = os_file.good();
            os_file.close();
            
            // Actually check if the file exists.
            if (file_exists)
            {
                cout << "Error, file already exists.";
                return;
            }
            else
            {
                // Open file for writing.
                os_file.open(copy_to, ios::out | ios::binary);
                if (!os_file.good())
                {
                    cout << "Error opening file for writing.\n";
                    return;
                }
                
                // Actually copy file out from the other file system.
                bool successful = file_system->file_read(os_file, copy_from);
                os_file.close();
                
                if (!successful)
                {
                    remove(copy_to.c_str());
                }
            }
        }
    }
    
    
    // @TODO format output neatly into appropriately sized columns -> function in utility?
    // @TODO sort vector by name
    // @TODO colorize: 
        // Yellow with black background: Device
        // Pink: Graphic image file
    void interface::command_ls(const string & switches)
    {
        vector<fs_entry_posix> file_listing = file_system->get_directory_contents();
        vector<string> filename_tokens;
        for (u32 i = 0; i < file_listing.size(); i++)
        {
            filename_tokens = utility::tokenize(file_listing[i].name, DELIMITER_DOT);
            string file_extension = (filename_tokens.size() > 1 ? filename_tokens.back() : ""); 
            
            if (switches == "-l" || switches == "-al"){
                if (switches == "-l" && (file_listing[i].name == "." || file_listing[i].name ==".."));
                else 
                    { // list permissions for each file
                if (file_listing[i].type == EXT2_DIR_TYPE_DIR)
                    cout << "d";
                else
                    cout << "-";
                if (file_listing[i].permissions & EXT2_INODE_PERM_USER_READ)
                    cout << "r";
                else 
                    cout << "-"; 
                if (file_listing[i].permissions & EXT2_INODE_PERM_USER_WRITE)
                    cout << "w";
                else 
                    cout << "-";
                if (file_listing[i].permissions & EXT2_INODE_PERM_USER_EXECUTE)
                    cout << "x";
                else 
                    cout << "-";
                if (file_listing[i].permissions & EXT2_INODE_PERM_GROUP_READ)
                    cout << "r";
                else 
                    cout << "-"; 
                if (file_listing[i].permissions & EXT2_INODE_PERM_GROUP_WRITE)
                    cout << "w";
                else 
                    cout << "-";
                if (file_listing[i].permissions & EXT2_INODE_PERM_GROUP_EXECUTE)
                    cout << "x";
                else 
                    cout << "-";
                if (file_listing[i].permissions & EXT2_INODE_PERM_OTHER_READ)
                    cout << "r";
                else 
                    cout << "-"; 
                if (file_listing[i].permissions & EXT2_INODE_PERM_OTHER_WRITE)
                    cout << "w";
                else 
                    cout << "-";
                if (file_listing[i].permissions & EXT2_INODE_PERM_OTHER_EXECUTE)
                    cout << "x ";
                else 
                    cout << "- ";

                // set file type, UID, GID, and size
                cout << setw(2) << ((file_listing[i].type== EXT2_DIR_TYPE_DIR)?2:1)<< " ";
                cout << setw(5) << file_listing[i].user_id<< " ";
                cout << setw(5) << file_listing[i].group_id << " ";
                cout << setw(10) << file_listing[i].size << " ";
                
                // translate unix epoch timestamps to readable time
                time_t     curr;
                struct tm  ts;
                char       buf[80];
                curr = file_listing[i].timestamp_modified;
                ts = *localtime(&curr);
                strftime(buf, sizeof(buf), "%b %d %M:%S", &ts);
                printf("%s ", buf);

                }
            }
            if (file_listing[i].type == EXT2_DIR_TYPE_DIR){
                if (switches != "-al" ){
                    if (file_listing[i].name == "." || file_listing[i].name ==".."); // don't print out . and .. directories with -l switch
                    else 
                        cout << "\033[1;34m" << file_listing[i].name << "\033[0m"<< "/"; //blue (bold) - directory or recognized data file                }
                }
                else if ((switches == "-al" ) && (file_listing[i].name == "." || file_listing[i].name ==".."))
                    cout << "\033[1;34m" << file_listing[i].name << "\033[0m";
                else 
                    cout << "\033[1;34m" << file_listing[i].name << "\033[0m" << "/"; //blue (bold) - directory or recognized data file
            }
            else if ((file_listing[i].type == EXT2_DIR_TYPE_FILE) &&
                    (file_listing[i].permissions & EXT2_INODE_PERM_USER_EXECUTE ||
                     file_listing[i].permissions & EXT2_INODE_PERM_GROUP_EXECUTE ||
                     file_listing[i].permissions & EXT2_INODE_PERM_OTHER_EXECUTE)){
                cout << "\033[1;32m" << file_listing[i].name << "\033[0m" << "*"; //green - executable files
                     }  
            else if (file_listing[i].type == EXT2_INODE_TYPE_SYMLINK)
                cout << "\033[6;36m" << file_listing[i].name << "\033[0m"; //cyan - linked file
            else if (file_listing[i].type == EXT2_INODE_TYPE_SYMLINK)
                cout << "\033[3;33m" << file_listing[i].name << "\033[0m"; //yellow (with black background) - device
            else if ((file_listing[i].type == EXT2_DIR_TYPE_FILE) &&  
                    (file_extension == "png" ||
                     file_extension == "jpg" ||
                     file_extension == "raw" ||
                     file_extension == "gif" ||
                     file_extension == "bmp" ||
                     file_extension == "tif")){ //38 - black
                cout << "\033[5;31m" << file_listing[i].name << "\033[0m"; }//pink - graphic image file
            else if ((file_listing[i].type == EXT2_DIR_TYPE_FILE) &&
                    (file_extension == "zip" ||
                     file_extension == "tar" ||
                     file_extension == "rar" ||
                     file_extension == "7z" ||
                     file_extension == "xz")){
                cout << "\033[0;31m" << file_listing[i].name << "\033[0m"; }//red - archive file
            else 
                cout << file_listing[i].name;
            
            if ((switches == "-l" && (file_listing[i].name != "." && file_listing[i].name !="..")) || switches == "-al")
                cout << "\n"; 
            else if (switches != "-al" && (file_listing[i].name == "." || file_listing[i].name ==".."));
            else
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