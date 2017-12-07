/*--------------------------------------------------------------------------------------------------
 * Author:      
 * Date:        2016-07-03
 * Assignment:  Final Project
 * Source File: ext2.cpp
 * Language:    C/C++
 * Course:      Operating Systems
 * Purpose:     Contains the implementation of the ext2 class.
 -------------------------------------------------------------------------------------------------*/

#include "constants.h"
#include "datatypes.h"
#include "ext2.h"
#include "utility.h"
#include "vdi_reader.h"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <ctime>

/*
 * EXT2 should read the MBR, boot sector, super block, and navigate through an EXT2 filesystem.
 */

using namespace std;
using namespace vdi_explorer;

namespace vdi_explorer
{
    /*----------------------------------------------------------------------------------------------
     * Name:    ext2
     * Type:    Function
     * Purpose: Constructor for the ext2 class. Reads in MBR and Superblock.
     * Input:   vdi_reader *_vdi, containing pointer to vdi object.
     * Output:  Nothing.
    ----------------------------------------------------------------------------------------------*/
    ext2::ext2(vdi_reader *_vdi)
    {
        // Store the pointer to the VDI reader and validate it.
        vdi = _vdi;
        if (!vdi)
        {
            cout << "Error opening the vdi reader object.";
            throw;
        }
        
        // Read the MBR boot sector.
        vdi->vdiSeek(0, SEEK_SET);
        vdi->vdiRead(&bootSector, 512);
        
        // Debug info.
        print_bootsector();
        // End debug info.
        
        // Determine the start of the superblock.
        superblock_start = bootSector.partitionTable[0].firstSector * VDI_SECTOR_SIZE +
                           EXT2_SUPERBLOCK_OFFSET;
        
        // Read the superblock.
        vdi->vdiSeek(superblock_start, SEEK_SET);
        vdi->vdiRead(&superblock, sizeof(ext2_superblock));
        
        // Record the actual block size.
        block_size_actual = EXT2_BLOCK_BASE_SIZE << superblock.s_log_block_size;
        
        // Calculate the max allowable file size.
        u16 dwords_per_block = block_size_actual / 4;
        u64 max_file_size_by_block = (dwords_per_block * dwords_per_block * dwords_per_block +
                                      dwords_per_block * dwords_per_block +
                                      dwords_per_block +
                                      EXT2_INODE_NBLOCKS_DIR) *
                                      block_size_actual;
        if (max_file_size_by_block < EXT2_MAX_ABS_FILE_SIZE)
            max_file_size = max_file_size_by_block;
        
        // Debug info.
        print_superblock();
        // End debug info.
        
        // Calculate and verify the number of block groups in the partition.
        u32 nBlockGroupCalc1, nBlockGroupCalc2;
        nBlockGroupCalc1 = (float)superblock.s_blocks_count / superblock.s_blocks_per_group + 0.5;
        nBlockGroupCalc2 = (float)superblock.s_inodes_count / superblock.s_inodes_per_group + 0.5;
        if (nBlockGroupCalc1 != nBlockGroupCalc2)
        {
            cout << "Block group calculation mismatch.";
            throw;
        }
        numBlockGroups = nBlockGroupCalc1;
        
        // Debug info.
        cout << "\n# of Block Groups:\n";
        cout << "Calculation 1: " << nBlockGroupCalc1 << endl;
        cout << "Calculation 2: " << nBlockGroupCalc2 << endl;
        // End debug info.
        
        // Allocate and verify an array of pointers to the (as yes unloaded) block group
        // descriptors.
        bgdTable = new ext2_block_group_desc[numBlockGroups];
        if (bgdTable == nullptr)
        {
            cout << "Error allocating the array of block group descriptors.";
            throw;
        }
        
        // Determine where the start of the block group descriptor table is.
        bgd_table_start = superblock.s_log_block_size == 0 ?
                          blockToOffset(2) :
                          blockToOffset(1);
        
        // Debug info.
        cout << "BGD Table Start: " << bgd_table_start << endl;
        // End debug info.
        
        // Read the block group descriptor table into memory.
        vdi->vdiSeek(bgd_table_start, SEEK_SET);
        vdi->vdiRead(bgdTable, sizeof(ext2_block_group_desc) * numBlockGroups);
        
        // Debug info.
        print_bgd_table();
        // End debug info.
        
        ext2_inode temp = readInode(2);//18);// 30481);
        
        // Debug info.
        print_inode(&temp);
        // End debug info.
        
        // u8 dummy = 0;
        // vdi->vdiSeek(blockToOffset(temp.i_block[0]), SEEK_SET);
        // vdi->vdiRead(&dummy, 1);
        
        auto temp2 = parse_directory_inode(temp);
        for (u32 i = 0; i < temp2.size(); i++)
            print_dir_entry(temp2[i], true);
        
        // set the root folder as the initial pwd.
        // pwd_inode.emplace_back(readInode(2));
        // pwd.emplace_back(parse_directory_inode(pwd_inode.back())[0]);
        pwd.emplace_back(parse_directory_inode(readInode(2))[0]);
        pwd.back().name.assign("/");
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    ~ext2
     * Type:    Function
     * Purpose: Destructor for the ext2 class.
     * Input:   Nothing.
     * Output:  Nothing.
    ----------------------------------------------------------------------------------------------*/
    ext2::~ext2()
    {
        // Delete the block descriptor table.
        if (bgdTable != nullptr)
            delete[] bgdTable;
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    list_directory_contents
     * Type:    Function
     * Purpose: Returns the contents of the current directory.
     * Input:   Nothing.
     * Output:  vector<fs_entry_posix>, containing a listing of all the files in the present working
     *          directory.
     *
     * @TODO    Optimize.
    ----------------------------------------------------------------------------------------------*/
    vector<fs_entry_posix> ext2::get_directory_contents(void)
    {
        vector<fs_entry_posix> to_return;
        ext2_inode temp_inode;
        
        // Parse the contents of the current directory.
        vector<ext2_dir_entry> directory_contents = parse_directory_inode(pwd.back().inode);
        
        // Instead of returning the raw contents, add a layer of abstraction to help facilitate
        // future design plans (eventual support for different filesystems).
        for (u32 i = 0; i < directory_contents.size(); i++)
        {
            // Read the entry's inode to gather data.
            temp_inode = readInode(directory_contents[i].inode);
            
            // Add an fs_entry_posix object to the back of the to_return vector and fill it with the
            // appropriate data.
            to_return.emplace_back();
            to_return.back().name = directory_contents[i].name;
            to_return.back().type = directory_contents[i].file_type;
            to_return.back().permissions = temp_inode.i_mode & 0x0fff; // mask for the bottom 12 bits
            to_return.back().user_id = temp_inode.i_uid;
            to_return.back().group_id = temp_inode.i_gid;
            to_return.back().size = temp_inode.i_size;
            to_return.back().timestamp_created = temp_inode.i_ctime;
            to_return.back().timestamp_modified = temp_inode.i_mtime;
        }
        
        return to_return;
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    get_pwd
     * Type:    Function
     * Purpose: Returns the path to and the name of the present working directory.
     * Input:   Nothing.
     * Output:  string, containing the path to and name of the present working directory.
    ----------------------------------------------------------------------------------------------*/
    string ext2::get_pwd()
    {
        string to_return;
        
        // Check if the pwd is the root directory.
        if (pwd.size() == 1)
        {
            to_return = "/";
        }
        else
        {
            // Otherwise, iterate through the path until the end, adding the name of the directories
            // to the string.
            for (u32 i = 1; i < pwd.size(); i++)
            {
                to_return += "/" + pwd[i].name;
            }
        }
        
        // Return the full path.
        return to_return;
    }

    
    /*----------------------------------------------------------------------------------------------
     * Name:    set_pwd
     * Type:    Function
     * Purpose: Sets the path to and the name of the present working directory.
     * Input:   string, containing the path to and name of the desired working directory.
     * Output:  Nothing.
     *
     * @TODO    Clean out dead code. / Make code look pretty.
    ----------------------------------------------------------------------------------------------*/
    void ext2::set_pwd(const string & desired_pwd)
    {
        // get pwd inode
        // parse directory inode
        // check returned results
        // if the returned results have a match to desired_pwd, change to that directory
        
        // u32 inode = pwd.back().inode;
        // vector<ext2_dir_entry> parsed_dir_inode = parse_directory_inode(inode);
        // for (u32 i = 0; i < parsed_dir_inode.size(); i++)
        // {
        //     if (parsed_dir_inode[i].file_type == 2 && parsed_dir_inode[i].name == desired_pwd)
        //         pwd.push_back(parsed_dir_inode[i]);
        // }
        
        vector<ext2_dir_entry> temp_pwd = dir_entry_exists(desired_pwd);
        if (temp_pwd.size() > 0)
            pwd = temp_pwd;
        
        // vector<string> tokens_desired = utility::tokenize(desired_pwd);
        // string to_return;
        // list<ext2_dir_entry>::iterator it = pwd.begin();
        
        // // Iterate through the path until the end, adding the name of the directories to the string.
        // while (it != pwd.end())
        // {
        //     to_return += "/" + it->name;
        //     it++;
        // }
        
        return;
    }

    
    /*----------------------------------------------------------------------------------------------
     * Name:    file_read
     * Type:    Function
     * Purpose: Reads a file from the virtual filesystem and outputs it on to the host filesystem.
     * Input:   fstream & output_file, contains the output file stream.
     * Input:   const string & file_to_read, holds the name of the file to read from the virtual
     *          filesystem.
     * Output:  bool, representing whether the file was found and read (true) or not (false).
    ----------------------------------------------------------------------------------------------*/
    bool ext2::file_read(fstream & output_file, const string & file_to_read)
    {
        // Check if the file specified in file_to_read exists or not, and if it does, its inode
        // number will be stored in file_inode.
        u32 file_inode = 0;
        if (file_entry_exists(file_to_read, file_inode) == true)
        {
            cout << "debug >> ext2::file_read >> file exists\n";
            
            // Get the file size.
            size_t file_size = readInode(file_inode).i_size;
            
            // Unroll the block chain where the file resides.
            list<u32> file_block_list = make_block_list(file_inode);
            cout << "debug (ext2::file_read) number of blocks: " << file_block_list.size() << endl;
            cin.ignore(1);
            
            // Set up an iterator to go through the block list.
            list<u32>::iterator iter = file_block_list.begin();
            
            // Set up some housekeeping variables.
            size_t bytes_read = 0;
            size_t bytes_to_read = 0;
            
            // Buffer to receive the file contents. Must be char (versus u8) or compiler pukes.
            char * read_buffer = new char[block_size_actual];

            // Loop until the whole file has been read.
            // @TODO determine if a (iter != file_block_list.end()) condition should be added.
            while (bytes_read < file_size)
            {
                // Determine how many bytes to read.  Read a full block if possible, otherwise read
                // just until the end of the file.
                bytes_to_read = (block_size_actual < file_size - bytes_read ? block_size_actual : file_size - bytes_read);
                
                // Set the VDI seek pointer to the apropriate position and then read the designated
                // number of bytes.
                vdi->vdiSeek(blockToOffset(*iter), SEEK_SET);
                vdi->vdiRead(read_buffer, bytes_to_read);
                
                // Immediately write the buffer to file.
                output_file.write(read_buffer, bytes_to_read);
                ///*--------------------------------------------------------------------------------------------------
 * Author:      
 * Date:        2016-07-03
 * Assignment:  Final Project
 * Source File: ext2.cpp
 * Language:    C/C++
 * Course:      Operating Systems
 * Purpose:     Contains the implementation of the ext2 class.
 -------------------------------------------------------------------------------------------------*/

#include "constants.h"
#include "datatypes.h"
#include "ext2.h"
#include "utility.h"
#include "vdi_reader.h"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <ctime>

/*
 * EXT2 should read the MBR, boot sector, super block, and navigate through an EXT2 filesystem.
 */

using namespace std;
using namespace vdi_explorer;

namespace vdi_explorer
{
    /*----------------------------------------------------------------------------------------------
     * Name:    ext2
     * Type:    Function
     * Purpose: Constructor for the ext2 class. Reads in MBR and Superblock.
     * Input:   vdi_reader *_vdi, containing pointer to vdi object.
     * Output:  Nothing.
    ----------------------------------------------------------------------------------------------*/
    ext2::ext2(vdi_reader *_vdi)
    {
        // Store the pointer to the VDI reader and validate it.
        vdi = _vdi;
        if (!vdi)
        {
            cout << "Error opening the vdi reader object.";
            throw;
        }
        
        // Read the MBR boot sector.
        vdi->vdiSeek(0, SEEK_SET);
        vdi->vdiRead(&bootSector, 512);
        
        // Debug info.
        print_bootsector();
        // End debug info.
        
        // Determine the start of the superblock.
        superblock_start = bootSector.partitionTable[0].firstSector * VDI_SECTOR_SIZE +
                           EXT2_SUPERBLOCK_OFFSET;
        
        // Read the superblock.
        vdi->vdiSeek(superblock_start, SEEK_SET);
        vdi->vdiRead(&superblock, sizeof(ext2_superblock));
        
        // Record the actual block size.
        block_size_actual = EXT2_BLOCK_BASE_SIZE << superblock.s_log_block_size;
        
        // Calculate the max allowable file size.
        u16 dwords_per_block = block_size_actual / 4;
        u64 max_file_size_by_block = (dwords_per_block * dwords_per_block * dwords_per_block +
                                      dwords_per_block * dwords_per_block +
                                      dwords_per_block +
                                      EXT2_INODE_NBLOCKS_DIR) *
                                      block_size_actual;
        if (max_file_size_by_block < EXT2_MAX_ABS_FILE_SIZE)
            max_file_size = max_file_size_by_block;
        
        // Debug info.
        print_superblock();
        // End debug info.
        
        // Calculate and verify the number of block groups in the partition.
        u32 nBlockGroupCalc1, nBlockGroupCalc2;
        nBlockGroupCalc1 = (float)superblock.s_blocks_count / superblock.s_blocks_per_group + 0.5;
        nBlockGroupCalc2 = (float)superblock.s_inodes_count / superblock.s_inodes_per_group + 0.5;
        if (nBlockGroupCalc1 != nBlockGroupCalc2)
        {
            cout << "Block group calculation mismatch.";
            throw;
        }
        numBlockGroups = nBlockGroupCalc1;
        
        // Debug info.
        cout << "\n# of Block Groups:\n";
        cout << "Calculation 1: " << nBlockGroupCalc1 << endl;
        cout << "Calculation 2: " << nBlockGroupCalc2 << endl;
        // End debug info.
        
        // Allocate and verify an array of pointers to the (as yes unloaded) block group
        // descriptors.
        bgdTable = new ext2_block_group_desc[numBlockGroups];
        if (bgdTable == nullptr)
        {
            cout << "Error allocating the array of block group descriptors.";
            throw;
        }
        
        // Determine where the start of the block group descriptor table is.
        bgd_table_start = superblock.s_log_block_size == 0 ?
                          blockToOffset(2) :
                          blockToOffset(1);
        
        // Debug info.
        cout << "BGD Table Start: " << bgd_table_start << endl;
        // End debug info.
        
        // Read the block group descriptor table into memory.
        vdi->vdiSeek(bgd_table_start, SEEK_SET);
        vdi->vdiRead(bgdTable, sizeof(ext2_block_group_desc) * numBlockGroups);
        
        // Debug info.
        print_bgd_table();
        // End debug info.
        
        ext2_inode temp = readInode(2);//18);// 30481);
        
        // Debug info.
        print_inode(&temp);
        // End debug info.
        
        // u8 dummy = 0;
        // vdi->vdiSeek(blockToOffset(temp.i_block[0]), SEEK_SET);
        // vdi->vdiRead(&dummy, 1);
        
        auto temp2 = parse_directory_inode(temp);
        for (u32 i = 0; i < temp2.size(); i++)
            print_dir_entry(temp2[i], true);
        
        // set the root folder as the initial pwd.
        // pwd_inode.emplace_back(readInode(2));
        // pwd.emplace_back(parse_directory_inode(pwd_inode.back())[0]);
        pwd.emplace_back(parse_directory_inode(readInode(2))[0]);
        pwd.back().name.assign("/");
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    ~ext2
     * Type:    Function
     * Purpose: Destructor for the ext2 class.
     * Input:   Nothing.
     * Output:  Nothing.
    ----------------------------------------------------------------------------------------------*/
    ext2::~ext2()
    {
        // Delete the block descriptor table.
        if (bgdTable != nullptr)
            delete[] bgdTable;
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    list_directory_contents
     * Type:    Function
     * Purpose: Returns the contents of the current directory.
     * Input:   Nothing.
     * Output:  vector<fs_entry_posix>, containing a listing of all the files in the present working
     *          directory.
     *
     * @TODO    Optimize.
    ----------------------------------------------------------------------------------------------*/
    vector<fs_entry_posix> ext2::get_directory_contents(void)
    {
        vector<fs_entry_posix> to_return;
        ext2_inode temp_inode;
        
        // Parse the contents of the current directory.
        vector<ext2_dir_entry> directory_contents = parse_directory_inode(pwd.back().inode);
        
        // Instead of returning the raw contents, add a layer of abstraction to help facilitate
        // future design plans (eventual support for different filesystems).
        for (u32 i = 0; i < directory_contents.size(); i++)
        {
            // Read the entry's inode to gather data.
            temp_inode = readInode(directory_contents[i].inode);
            
            // Add an fs_entry_posix object to the back of the to_return vector and fill it with the
            // appropriate data.
            to_return.emplace_back();
            to_return.back().name = directory_contents[i].name;
            to_return.back().type = directory_contents[i].file_type;
            to_return.back().permissions = temp_inode.i_mode & 0x0fff; // mask for the bottom 12 bits
            to_return.back().user_id = temp_inode.i_uid;
            to_return.back().group_id = temp_inode.i_gid;
            to_return.back().size = temp_inode.i_size;
            to_return.back().timestamp_created = temp_inode.i_ctime;
            to_return.back().timestamp_modified = temp_inode.i_mtime;
        }
        
        return to_return;
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    get_pwd
     * Type:    Function
     * Purpose: Returns the path to and the name of the present working directory.
     * Input:   Nothing.
     * Output:  string, containing the path to and name of the present working directory.
    ----------------------------------------------------------------------------------------------*/
    string ext2::get_pwd()
    {
        string to_return;
        
        // Check if the pwd is the root directory.
        if (pwd.size() == 1)
        {
            to_return = "/";
        }
        else
        {
            // Otherwise, iterate through the path until the end, adding the name of the directories
            // to the string.
            for (u32 i = 1; i < pwd.size(); i++)
            {
                to_return += "/" + pwd[i].name;
            }
        }
        
        // Return the full path.
        return to_return;
    }

    
    /*----------------------------------------------------------------------------------------------
     * Name:    set_pwd
     * Type:    Function
     * Purpose: Sets the path to and the name of the present working directory.
     * Input:   string, containing the path to and name of the desired working directory.
     * Output:  Nothing.
     *
     * @TODO    Clean out dead code. / Make code look pretty.
    ----------------------------------------------------------------------------------------------*/
    void ext2::set_pwd(const string & desired_pwd)
    {
        // get pwd inode
        // parse directory inode
        // check returned results
        // if the returned results have a match to desired_pwd, change to that directory
        
        // u32 inode = pwd.back().inode;
        // vector<ext2_dir_entry> parsed_dir_inode = parse_directory_inode(inode);
        // for (u32 i = 0; i < parsed_dir_inode.size(); i++)
        // {
        //     if (parsed_dir_inode[i].file_type == 2 && parsed_dir_inode[i].name == desired_pwd)
        //         pwd.push_back(parsed_dir_inode[i]);
        // }
        
        vector<ext2_dir_entry> temp_pwd = dir_entry_exists(desired_pwd);
        if (temp_pwd.size() > 0)
            pwd = temp_pwd;
        
        // vector<string> tokens_desired = utility::tokenize(desired_pwd);
        // string to_return;
        // list<ext2_dir_entry>::iterator it = pwd.begin();
        
        // // Iterate through the path until the end, adding the name of the directories to the string.
        // while (it != pwd.end())
        // {
        //     to_return += "/" + it->name;
        //     it++;
        // }
        
        return;
    }

    
    /*----------------------------------------------------------------------------------------------
     * Name:    file_read
     * Type:    Function
     * Purpose: Reads a file from the virtual filesystem and outputs it on to the host filesystem.
     * Input:   fstream & output_file, contains the output file stream.
     * Input:   const string & file_to_read, holds the name of the file to read from the virtual
     *          filesystem.
     * Output:  bool, representing whether the file was found and read (true) or not (false).
    ----------------------------------------------------------------------------------------------*/
    bool ext2::file_read(fstream & output_file, const string & file_to_read)
    {
        // Check if the file specified in file_to_read exists or not, and if it does, its inode
        // number will be stored in file_inode.
        u32 file_inode = 0;
        if (file_entry_exists(file_to_read, file_inode) == true)
        {
            cout << "debug >> ext2::file_read >> file exists\n";
            
            // Get the file size.
            size_t file_size = readInode(file_inode).i_size;
            
            // Unroll the block chain where the file resides.
            list<u32> file_block_list = make_block_list(file_inode);
            cout << "debug (ext2::file_read) number of blocks: " << file_block_list.size() << endl;
            cin.ignore(1);
            
            // Set up an iterator to go through the block list.
            list<u32>::iterator iter = file_block_list.begin();
            
            // Set up some housekeeping variables.
            size_t bytes_read = 0;
            size_t bytes_to_read = 0;
            
            // Buffer to receive the file contents. Must be char (versus u8) or compiler pukes.
            char * read_buffer = new char[block_size_actual];

            // Loop until the whole file has been read.
            // @TODO determine if a (iter != file_block_list.end()) condition should be added.
            while (bytes_read < file_size)
            {
                // Determine how many bytes to read.  Read a full block if possible, otherwise read
                // just until the end of the file.
                bytes_to_read = (block_size_actual < file_size - bytes_read ? block_size_actual : file_size - bytes_read);
                
                // Set the VDI seek pointer to the apropriate position and then read the designated
                // number of bytes.
                vdi->vdiSeek(blockToOffset(*iter), SEEK_SET);
                vdi->vdiRead(read_buffer, bytes_to_read);
                
                // Immediately write the buffer to file.
                output_file.write(read_buffer, bytes_to_read);
                ///*--------------------------------------------------------------------------------------------------
 * Author:      
 * Date:        2016-07-03
 * Assignment:  Final Project
 * Source File: ext2.cpp
 * Language:    C/C++
 * Course:      Operating Systems
 * Purpose:     Contains the implementation of the ext2 class.
 -------------------------------------------------------------------------------------------------*/

#include "constants.h"
#include "datatypes.h"
#include "ext2.h"
#include "utility.h"
#include "vdi_reader.h"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <ctime>

/*
 * EXT2 should read the MBR, boot sector, super block, and navigate through an EXT2 filesystem.
 */

using namespace std;
using namespace vdi_explorer;

namespace vdi_explorer
{
    /*----------------------------------------------------------------------------------------------
     * Name:    ext2
     * Type:    Function
     * Purpose: Constructor for the ext2 class. Reads in MBR and Superblock.
     * Input:   vdi_reader *_vdi, containing pointer to vdi object.
     * Output:  Nothing.
    ----------------------------------------------------------------------------------------------*/
    ext2::ext2(vdi_reader *_vdi)
    {
        // Store the pointer to the VDI reader and validate it.
        vdi = _vdi;
        if (!vdi)
        {
            cout << "Error opening the vdi reader object.";
            throw;
        }
        
        // Read the MBR boot sector.
        vdi->vdiSeek(0, SEEK_SET);
        vdi->vdiRead(&bootSector, 512);
        
        // Debug info.
        print_bootsector();
        // End debug info.
        
        // Determine the start of the superblock.
        superblock_start = bootSector.partitionTable[0].firstSector * VDI_SECTOR_SIZE +
                           EXT2_SUPERBLOCK_OFFSET;
        
        // Read the superblock.
        vdi->vdiSeek(superblock_start, SEEK_SET);
        vdi->vdiRead(&superblock, sizeof(ext2_superblock));
        
        // Record the actual block size.
        block_size_actual = EXT2_BLOCK_BASE_SIZE << superblock.s_log_block_size;
        
        // Calculate the max allowable file size.
        u16 dwords_per_block = block_size_actual / 4;
        u64 max_file_size_by_block = (dwords_per_block * dwords_per_block * dwords_per_block +
                                      dwords_per_block * dwords_per_block +
                                      dwords_per_block +
                                      EXT2_INODE_NBLOCKS_DIR) *
                                      block_size_actual;
        if (max_file_size_by_block < EXT2_MAX_ABS_FILE_SIZE)
            max_file_size = max_file_size_by_block;
        
        // Debug info.
        print_superblock();
        // End debug info.
        
        // Calculate and verify the number of block groups in the partition.
        u32 nBlockGroupCalc1, nBlockGroupCalc2;
        nBlockGroupCalc1 = (float)superblock.s_blocks_count / superblock.s_blocks_per_group + 0.5;
        nBlockGroupCalc2 = (float)superblock.s_inodes_count / superblock.s_inodes_per_group + 0.5;
        if (nBlockGroupCalc1 != nBlockGroupCalc2)
        {
            cout << "Block group calculation mismatch.";
            throw;
        }
        numBlockGroups = nBlockGroupCalc1;
        
        // Debug info.
        cout << "\n# of Block Groups:\n";
        cout << "Calculation 1: " << nBlockGroupCalc1 << endl;
        cout << "Calculation 2: " << nBlockGroupCalc2 << endl;
        // End debug info.
        
        // Allocate and verify an array of pointers to the (as yes unloaded) block group
        // descriptors.
        bgdTable = new ext2_block_group_desc[numBlockGroups];
        if (bgdTable == nullptr)
        {
            cout << "Error allocating the array of block group descriptors.";
            throw;
        }
        
        // Determine where the start of the block group descriptor table is.
        bgd_table_start = superblock.s_log_block_size == 0 ?
                          blockToOffset(2) :
                          blockToOffset(1);
        
        // Debug info.
        cout << "BGD Table Start: " << bgd_table_start << endl;
        // End debug info.
        
        // Read the block group descriptor table into memory.
        vdi->vdiSeek(bgd_table_start, SEEK_SET);
        vdi->vdiRead(bgdTable, sizeof(ext2_block_group_desc) * numBlockGroups);
        
        // Debug info.
        print_bgd_table();
        // End debug info.
        
        ext2_inode temp = readInode(2);//18);// 30481);
        
        // Debug info.
        print_inode(&temp);
        // End debug info.
        
        // u8 dummy = 0;
        // vdi->vdiSeek(blockToOffset(temp.i_block[0]), SEEK_SET);
        // vdi->vdiRead(&dummy, 1);
        
        auto temp2 = parse_directory_inode(temp);
        for (u32 i = 0; i < temp2.size(); i++)
            print_dir_entry(temp2[i], true);
        
        // set the root folder as the initial pwd.
        // pwd_inode.emplace_back(readInode(2));
        // pwd.emplace_back(parse_directory_inode(pwd_inode.back())[0]);
        pwd.emplace_back(parse_directory_inode(readInode(2))[0]);
        pwd.back().name.assign("/");
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    ~ext2
     * Type:    Function
     * Purpose: Destructor for the ext2 class.
     * Input:   Nothing.
     * Output:  Nothing.
    ----------------------------------------------------------------------------------------------*/
    ext2::~ext2()
    {
        // Delete the block descriptor table.
        if (bgdTable != nullptr)
            delete[] bgdTable;
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    list_directory_contents
     * Type:    Function
     * Purpose: Returns the contents of the current directory.
     * Input:   Nothing.
     * Output:  vector<fs_entry_posix>, containing a listing of all the files in the present working
     *          directory.
     *
     * @TODO    Optimize.
    ----------------------------------------------------------------------------------------------*/
    vector<fs_entry_posix> ext2::get_directory_contents(void)
    {
        vector<fs_entry_posix> to_return;
        ext2_inode temp_inode;
        
        // Parse the contents of the current directory.
        vector<ext2_dir_entry> directory_contents = parse_directory_inode(pwd.back().inode);
        
        // Instead of returning the raw contents, add a layer of abstraction to help facilitate
        // future design plans (eventual support for different filesystems).
        for (u32 i = 0; i < directory_contents.size(); i++)
        {
            // Read the entry's inode to gather data.
            temp_inode = readInode(directory_contents[i].inode);
            
            // Add an fs_entry_posix object to the back of the to_return vector and fill it with the
            // appropriate data.
            to_return.emplace_back();
            to_return.back().name = directory_contents[i].name;
            to_return.back().type = directory_contents[i].file_type;
            to_return.back().permissions = temp_inode.i_mode & 0x0fff; // mask for the bottom 12 bits
            to_return.back().user_id = temp_inode.i_uid;
            to_return.back().group_id = temp_inode.i_gid;
            to_return.back().size = temp_inode.i_size;
            to_return.back().timestamp_created = temp_inode.i_ctime;
            to_return.back().timestamp_modified = temp_inode.i_mtime;
        }
        
        return to_return;
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    get_pwd
     * Type:    Function
     * Purpose: Returns the path to and the name of the present working directory.
     * Input:   Nothing.
     * Output:  string, containing the path to and name of the present working directory.
    ----------------------------------------------------------------------------------------------*/
    string ext2::get_pwd()
    {
        string to_return;
        
        // Check if the pwd is the root directory.
        if (pwd.size() == 1)
        {
            to_return = "/";
        }
        else
        {
            // Otherwise, iterate through the path until the end, adding the name of the directories
            // to the string.
            for (u32 i = 1; i < pwd.size(); i++)
            {
                to_return += "/" + pwd[i].name;
            }
        }
        
        // Return the full path.
        return to_return;
    }

    
    /*----------------------------------------------------------------------------------------------
     * Name:    set_pwd
     * Type:    Function
     * Purpose: Sets the path to and the name of the present working directory.
     * Input:   string, containing the path to and name of the desired working directory.
     * Output:  Nothing.
     *
     * @TODO    Clean out dead code. / Make code look pretty.
    ----------------------------------------------------------------------------------------------*/
    void ext2::set_pwd(const string & desired_pwd)
    {
        // get pwd inode
        // parse directory inode
        // check returned results
        // if the returned results have a match to desired_pwd, change to that directory
        
        // u32 inode = pwd.back().inode;
        // vector<ext2_dir_entry> parsed_dir_inode = parse_directory_inode(inode);
        // for (u32 i = 0; i < parsed_dir_inode.size(); i++)
        // {
        //     if (parsed_dir_inode[i].file_type == 2 && parsed_dir_inode[i].name == desired_pwd)
        //         pwd.push_back(parsed_dir_inode[i]);
        // }
        
        vector<ext2_dir_entry> temp_pwd = dir_entry_exists(desired_pwd);
        if (temp_pwd.size() > 0)
            pwd = temp_pwd;
        
        // vector<string> tokens_desired = utility::tokenize(desired_pwd);
        // string to_return;
        // list<ext2_dir_entry>::iterator it = pwd.begin();
        
        // // Iterate through the path until the end, adding the name of the directories to the string.
        // while (it != pwd.end())
        // {
        //     to_return += "/" + it->name;
        //     it++;
        // }
        
        return;
    }

    
    /*----------------------------------------------------------------------------------------------
     * Name:    file_read
     * Type:    Function
     * Purpose: Reads a file from the virtual filesystem and outputs it on to the host filesystem.
     * Input:   fstream & output_file, contains the output file stream.
     * Input:   const string & file_to_read, holds the name of the file to read from the virtual
     *          filesystem.
     * Output:  bool, representing whether the file was found and read (true) or not (false).
    ----------------------------------------------------------------------------------------------*/
    bool ext2::file_read(fstream & output_file, const string & file_to_read)
    {
        // Check if the file specified in file_to_read exists or not, and if it does, its inode
        // number will be stored in file_inode.
        u32 file_inode = 0;
        if (file_entry_exists(file_to_read, file_inode) == true)
        {
            cout << "debug >> ext2::file_read >> file exists\n";
            
            // Get the file size.
            size_t file_size = readInode(file_inode).i_size;
            
            // Unroll the block chain where the file resides.
            list<u32> file_block_list = make_block_list(file_inode);
            cout << "debug (ext2::file_read) number of blocks: " << file_block_list.size() << endl;
            cin.ignore(1);
            
            // Set up an iterator to go through the block list.
            list<u32>::iterator iter = file_block_list.begin();
            
            // Set up some housekeeping variables.
            size_t bytes_read = 0;
            size_t bytes_to_read = 0;
            
            // Buffer to receive the file contents. Must be char (versus u8) or compiler pukes.
            char * read_buffer = new char[block_size_actual];

            // Loop until the whole file has been read.
            // @TODO determine if a (iter != file_block_list.end()) condition should be added.
            while (bytes_read < file_size)
            {
                // Determine how many bytes to read.  Read a full block if possible, otherwise read
                // just until the end of the file.
                bytes_to_read = (block_size_actual < file_size - bytes_read ? block_size_actual : file_size - bytes_read);
                
                // Set the VDI seek pointer to the apropriate position and then read the designated
                // number of bytes.
                vdi->vdiSeek(blockToOffset(*iter), SEEK_SET);
                vdi->vdiRead(read_buffer, bytes_to_read);
                
                // Immediately write the buffer to file.
                output_file.write(read_buffer, bytes_to_read);
                ///*--------------------------------------------------------------------------------------------------
 * Author:      
 * Date:        2016-07-03
 * Assignment:  Final Project
 * Source File: ext2.cpp
 * Language:    C/C++
 * Course:      Operating Systems
 * Purpose:     Contains the implementation of the ext2 class.
 -------------------------------------------------------------------------------------------------*/

#include "constants.h"
#include "datatypes.h"
#include "ext2.h"
#include "utility.h"
#include "vdi_reader.h"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <ctime>

/*
 * EXT2 should read the MBR, boot sector, super block, and navigate through an EXT2 filesystem.
 */

using namespace std;
using namespace vdi_explorer;

namespace vdi_explorer
{
    /*----------------------------------------------------------------------------------------------
     * Name:    ext2
     * Type:    Function
     * Purpose: Constructor for the ext2 class. Reads in MBR and Superblock.
     * Input:   vdi_reader *_vdi, containing pointer to vdi object.
     * Output:  Nothing.
    ----------------------------------------------------------------------------------------------*/
    ext2::ext2(vdi_reader *_vdi)
    {
        // Store the pointer to the VDI reader and validate it.
        vdi = _vdi;
        if (!vdi)
        {
            cout << "Error opening the vdi reader object.";
            throw;
        }
        
        // Read the MBR boot sector.
        vdi->vdiSeek(0, SEEK_SET);
        vdi->vdiRead(&bootSector, 512);
        
        // Debug info.
        print_bootsector();
        // End debug info.
        
        // Determine the start of the superblock.
        superblock_start = bootSector.partitionTable[0].firstSector * VDI_SECTOR_SIZE +
                           EXT2_SUPERBLOCK_OFFSET;
        
        // Read the superblock.
        vdi->vdiSeek(superblock_start, SEEK_SET);
        vdi->vdiRead(&superblock, sizeof(ext2_superblock));
        
        // Record the actual block size.
        block_size_actual = EXT2_BLOCK_BASE_SIZE << superblock.s_log_block_size;
        
        // Calculate the max allowable file size.
        u16 dwords_per_block = block_size_actual / 4;
        u64 max_file_size_by_block = (dwords_per_block * dwords_per_block * dwords_per_block +
                                      dwords_per_block * dwords_per_block +
                                      dwords_per_block +
                                      EXT2_INODE_NBLOCKS_DIR) *
                                      block_size_actual;
        if (max_file_size_by_block < EXT2_MAX_ABS_FILE_SIZE)
            max_file_size = max_file_size_by_block;
        
        // Debug info.
        print_superblock();
        // End debug info.
        
        // Calculate and verify the number of block groups in the partition.
        u32 nBlockGroupCalc1, nBlockGroupCalc2;
        nBlockGroupCalc1 = (float)superblock.s_blocks_count / superblock.s_blocks_per_group + 0.5;
        nBlockGroupCalc2 = (float)superblock.s_inodes_count / superblock.s_inodes_per_group + 0.5;
        if (nBlockGroupCalc1 != nBlockGroupCalc2)
        {
            cout << "Block group calculation mismatch.";
            throw;
        }
        numBlockGroups = nBlockGroupCalc1;
        
        // Debug info.
        cout << "\n# of Block Groups:\n";
        cout << "Calculation 1: " << nBlockGroupCalc1 << endl;
        cout << "Calculation 2: " << nBlockGroupCalc2 << endl;
        // End debug info.
        
        // Allocate and verify an array of pointers to the (as yes unloaded) block group
        // descriptors.
        bgdTable = new ext2_block_group_desc[numBlockGroups];
        if (bgdTable == nullptr)
        {
            cout << "Error allocating the array of block group descriptors.";
            throw;
        }
        
        // Determine where the start of the block group descriptor table is.
        bgd_table_start = superblock.s_log_block_size == 0 ?
                          blockToOffset(2) :
                          blockToOffset(1);
        
        // Debug info.
        cout << "BGD Table Start: " << bgd_table_start << endl;
        // End debug info.
        
        // Read the block group descriptor table into memory.
        vdi->vdiSeek(bgd_table_start, SEEK_SET);
        vdi->vdiRead(bgdTable, sizeof(ext2_block_group_desc) * numBlockGroups);
        
        // Debug info.
        print_bgd_table();
        // End debug info.
        
        ext2_inode temp = readInode(2);//18);// 30481);
        
        // Debug info.
        print_inode(&temp);
        // End debug info.
        
        // u8 dummy = 0;
        // vdi->vdiSeek(blockToOffset(temp.i_block[0]), SEEK_SET);
        // vdi->vdiRead(&dummy, 1);
        
        auto temp2 = parse_directory_inode(temp);
        for (u32 i = 0; i < temp2.size(); i++)
            print_dir_entry(temp2[i], true);
        
        // set the root folder as the initial pwd.
        // pwd_inode.emplace_back(readInode(2));
        // pwd.emplace_back(parse_directory_inode(pwd_inode.back())[0]);
        pwd.emplace_back(parse_directory_inode(readInode(2))[0]);
        pwd.back().name.assign("/");
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    ~ext2
     * Type:    Function
     * Purpose: Destructor for the ext2 class.
     * Input:   Nothing.
     * Output:  Nothing.
    ----------------------------------------------------------------------------------------------*/
    ext2::~ext2()
    {
        // Delete the block descriptor table.
        if (bgdTable != nullptr)
            delete[] bgdTable;
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    list_directory_contents
     * Type:    Function
     * Purpose: Returns the contents of the current directory.
     * Input:   Nothing.
     * Output:  vector<fs_entry_posix>, containing a listing of all the files in the present working
     *          directory.
     *
     * @TODO    Optimize.
    ----------------------------------------------------------------------------------------------*/
    vector<fs_entry_posix> ext2::get_directory_contents(void)
    {
        vector<fs_entry_posix> to_return;
        ext2_inode temp_inode;
        
        // Parse the contents of the current directory.
        vector<ext2_dir_entry> directory_contents = parse_directory_inode(pwd.back().inode);
        
        // Instead of returning the raw contents, add a layer of abstraction to help facilitate
        // future design plans (eventual support for different filesystems).
        for (u32 i = 0; i < directory_contents.size(); i++)
        {
            // Read the entry's inode to gather data.
            temp_inode = readInode(directory_contents[i].inode);
            
            // Add an fs_entry_posix object to the back of the to_return vector and fill it with the
            // appropriate data.
            to_return.emplace_back();
            to_return.back().name = directory_contents[i].name;
            to_return.back().type = directory_contents[i].file_type;
            to_return.back().permissions = temp_inode.i_mode & 0x0fff; // mask for the bottom 12 bits
            to_return.back().user_id = temp_inode.i_uid;
            to_return.back().group_id = temp_inode.i_gid;
            to_return.back().size = temp_inode.i_size;
            to_return.back().timestamp_created = temp_inode.i_ctime;
            to_return.back().timestamp_modified = temp_inode.i_mtime;
        }
        
        return to_return;
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    get_pwd
     * Type:    Function
     * Purpose: Returns the path to and the name of the present working directory.
     * Input:   Nothing.
     * Output:  string, containing the path to and name of the present working directory.
    ----------------------------------------------------------------------------------------------*/
    string ext2::get_pwd()
    {
        string to_return;
        
        // Check if the pwd is the root directory.
        if (pwd.size() == 1)
        {
            to_return = "/";
        }
        else
        {
            // Otherwise, iterate through the path until the end, adding the name of the directories
            // to the string.
            for (u32 i = 1; i < pwd.size(); i++)
            {
                to_return += "/" + pwd[i].name;
            }
        }
        
        // Return the full path.
        return to_return;
    }

    
    /*----------------------------------------------------------------------------------------------
     * Name:    set_pwd
     * Type:    Function
     * Purpose: Sets the path to and the name of the present working directory.
     * Input:   string, containing the path to and name of the desired working directory.
     * Output:  Nothing.
     *
     * @TODO    Clean out dead code. / Make code look pretty.
    ----------------------------------------------------------------------------------------------*/
    void ext2::set_pwd(const string & desired_pwd)
    {
        // get pwd inode
        // parse directory inode
        // check returned results
        // if the returned results have a match to desired_pwd, change to that directory
        
        // u32 inode = pwd.back().inode;
        // vector<ext2_dir_entry> parsed_dir_inode = parse_directory_inode(inode);
        // for (u32 i = 0; i < parsed_dir_inode.size(); i++)
        // {
        //     if (parsed_dir_inode[i].file_type == 2 && parsed_dir_inode[i].name == desired_pwd)
        //         pwd.push_back(parsed_dir_inode[i]);
        // }
        
        vector<ext2_dir_entry> temp_pwd = dir_entry_exists(desired_pwd);
        if (temp_pwd.size() > 0)
            pwd = temp_pwd;
        
        // vector<string> tokens_desired = utility::tokenize(desired_pwd);
        // string to_return;
        // list<ext2_dir_entry>::iterator it = pwd.begin();
        
        // // Iterate through the path until the end, adding the name of the directories to the string.
        // while (it != pwd.end())
        // {
        //     to_return += "/" + it->name;
        //     it++;
        // }
        
        return;
    }

    
    /*----------------------------------------------------------------------------------------------
     * Name:    file_read
     * Type:    Function
     * Purpose: Reads a file from the virtual filesystem and outputs it on to the host filesystem.
     * Input:   fstream & output_file, contains the output file stream.
     * Input:   const string & file_to_read, holds the name of the file to read from the virtual
     *          filesystem.
     * Output:  bool, representing whether the file was found and read (true) or not (false).
    ----------------------------------------------------------------------------------------------*/
    bool ext2::file_read(fstream & output_file, const string & file_to_read)
    {
        // Check if the file specified in file_to_read exists or not, and if it does, its inode
        // number will be stored in file_inode.
        u32 file_inode = 0;
        if (file_entry_exists(file_to_read, file_inode) == true)
        {
            cout << "debug >> ext2::file_read >> file exists\n";
            
            // Get the file size.
            size_t file_size = readInode(file_inode).i_size;
            
            // Unroll the block chain where the file resides.
            list<u32> file_block_list = make_block_list(file_inode);
            cout << "debug (ext2::file_read) number of blocks: " << file_block_list.size() << endl;
            cin.ignore(1);
            
            // Set up an iterator to go through the block list.
            list<u32>::iterator iter = file_block_list.begin();
            
            // Set up some housekeeping variables.
            size_t bytes_read = 0;
            size_t bytes_to_read = 0;
            
            // Buffer to receive the file contents. Must be char (versus u8) or compiler pukes.
            char * read_buffer = new char[block_size_actual];

            // Loop until the whole file has been read.
            // @TODO determine if a (iter != file_block_list.end()) condition should be added.
            while (bytes_read < file_size)
            {
                // Determine how many bytes to read.  Read a full block if possible, otherwise read
                // just until the end of the file.
                bytes_to_read = (block_size_actual < file_size - bytes_read ? block_size_actual : file_size - bytes_read);
                
                // Set the VDI seek pointer to the apropriate position and then read the designated
                // number of bytes.
                vdi->vdiSeek(blockToOffset(*iter), SEEK_SET);
                vdi->vdiRead(read_buffer, bytes_to_read);
                
                // Immediately write the buffer to file.
                output_file.write(read_buffer, bytes_to_read);
                ///*--------------------------------------------------------------------------------------------------
 * Author:      
 * Date:        2016-07-03
 * Assignment:  Final Project
 * Source File: ext2.cpp
 * Language:    C/C++
 * Course:      Operating Systems
 * Purpose:     Contains the implementation of the ext2 class.
 -------------------------------------------------------------------------------------------------*/

#include "constants.h"
#include "datatypes.h"
#include "ext2.h"
#include "utility.h"
#include "vdi_reader.h"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <ctime>

/*
 * EXT2 should read the MBR, boot sector, super block, and navigate through an EXT2 filesystem.
 */

using namespace std;
using namespace vdi_explorer;

namespace vdi_explorer
{
    /*----------------------------------------------------------------------------------------------
     * Name:    ext2
     * Type:    Function
     * Purpose: Constructor for the ext2 class. Reads in MBR and Superblock.
     * Input:   vdi_reader *_vdi, containing pointer to vdi object.
     * Output:  Nothing.
    ----------------------------------------------------------------------------------------------*/
    ext2::ext2(vdi_reader *_vdi)
    {
        // Store the pointer to the VDI reader and validate it.
        vdi = _vdi;
        if (!vdi)
        {
            cout << "Error opening the vdi reader object.";
            throw;
        }
        
        // Read the MBR boot sector.
        vdi->vdiSeek(0, SEEK_SET);
        vdi->vdiRead(&bootSector, 512);
        
        // Debug info.
        print_bootsector();
        // End debug info.
        
        // Determine the start of the superblock.
        superblock_start = bootSector.partitionTable[0].firstSector * VDI_SECTOR_SIZE +
                           EXT2_SUPERBLOCK_OFFSET;
        
        // Read the superblock.
        vdi->vdiSeek(superblock_start, SEEK_SET);
        vdi->vdiRead(&superblock, sizeof(ext2_superblock));
        
        // Record the actual block size.
        block_size_actual = EXT2_BLOCK_BASE_SIZE << superblock.s_log_block_size;
        
        // Calculate the max allowable file size.
        u16 dwords_per_block = block_size_actual / 4;
        u64 max_file_size_by_block = (dwords_per_block * dwords_per_block * dwords_per_block +
                                      dwords_per_block * dwords_per_block +
                                      dwords_per_block +
                                      EXT2_INODE_NBLOCKS_DIR) *
                                      block_size_actual;
        if (max_file_size_by_block < EXT2_MAX_ABS_FILE_SIZE)
            max_file_size = max_file_size_by_block;
        
        // Debug info.
        print_superblock();
        // End debug info.
        
        // Calculate and verify the number of block groups in the partition.
        u32 nBlockGroupCalc1, nBlockGroupCalc2;
        nBlockGroupCalc1 = (float)superblock.s_blocks_count / superblock.s_blocks_per_group + 0.5;
        nBlockGroupCalc2 = (float)superblock.s_inodes_count / superblock.s_inodes_per_group + 0.5;
        if (nBlockGroupCalc1 != nBlockGroupCalc2)
        {
            cout << "Block group calculation mismatch.";
            throw;
        }
        numBlockGroups = nBlockGroupCalc1;
        
        // Debug info.
        cout << "\n# of Block Groups:\n";
        cout << "Calculation 1: " << nBlockGroupCalc1 << endl;
        cout << "Calculation 2: " << nBlockGroupCalc2 << endl;
        // End debug info.
        
        // Allocate and verify an array of pointers to the (as yes unloaded) block group
        // descriptors.
        bgdTable = new ext2_block_group_desc[numBlockGroups];
        if (bgdTable == nullptr)
        {
            cout << "Error allocating the array of block group descriptors.";
            throw;
        }
        
        // Determine where the start of the block group descriptor table is.
        bgd_table_start = superblock.s_log_block_size == 0 ?
                          blockToOffset(2) :
                          blockToOffset(1);
        
        // Debug info.
        cout << "BGD Table Start: " << bgd_table_start << endl;
        // End debug info.
        
        // Read the block group descriptor table into memory.
        vdi->vdiSeek(bgd_table_start, SEEK_SET);
        vdi->vdiRead(bgdTable, sizeof(ext2_block_group_desc) * numBlockGroups);
        
        // Debug info.
        print_bgd_table();
        // End debug info.
        
        ext2_inode temp = readInode(2);//18);// 30481);
        
        // Debug info.
        print_inode(&temp);
        // End debug info.
        
        // u8 dummy = 0;
        // vdi->vdiSeek(blockToOffset(temp.i_block[0]), SEEK_SET);
        // vdi->vdiRead(&dummy, 1);
        
        auto temp2 = parse_directory_inode(temp);
        for (u32 i = 0; i < temp2.size(); i++)
            print_dir_entry(temp2[i], true);
        
        // set the root folder as the initial pwd.
        // pwd_inode.emplace_back(readInode(2));
        // pwd.emplace_back(parse_directory_inode(pwd_inode.back())[0]);
        pwd.emplace_back(parse_directory_inode(readInode(2))[0]);
        pwd.back().name.assign("/");
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    ~ext2
     * Type:    Function
     * Purpose: Destructor for the ext2 class.
     * Input:   Nothing.
     * Output:  Nothing.
    ----------------------------------------------------------------------------------------------*/
    ext2::~ext2()
    {
        // Delete the block descriptor table.
        if (bgdTable != nullptr)
            delete[] bgdTable;
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    list_directory_contents
     * Type:    Function
     * Purpose: Returns the contents of the current directory.
     * Input:   Nothing.
     * Output:  vector<fs_entry_posix>, containing a listing of all the files in the present working
     *          directory.
     *
     * @TODO    Optimize.
    ----------------------------------------------------------------------------------------------*/
    vector<fs_entry_posix> ext2::get_directory_contents(void)
    {
        vector<fs_entry_posix> to_return;
        ext2_inode temp_inode;
        
        // Parse the contents of the current directory.
        vector<ext2_dir_entry> directory_contents = parse_directory_inode(pwd.back().inode);
        
        // Instead of returning the raw contents, add a layer of abstraction to help facilitate
        // future design plans (eventual support for different filesystems).
        for (u32 i = 0; i < directory_contents.size(); i++)
        {
            // Read the entry's inode to gather data.
            temp_inode = readInode(directory_contents[i].inode);
            
            // Add an fs_entry_posix object to the back of the to_return vector and fill it with the
            // appropriate data.
            to_return.emplace_back();
            to_return.back().name = directory_contents[i].name;
            to_return.back().type = directory_contents[i].file_type;
            to_return.back().permissions = temp_inode.i_mode & 0x0fff; // mask for the bottom 12 bits
            to_return.back().user_id = temp_inode.i_uid;
            to_return.back().group_id = temp_inode.i_gid;
            to_return.back().size = temp_inode.i_size;
            to_return.back().timestamp_created = temp_inode.i_ctime;
            to_return.back().timestamp_modified = temp_inode.i_mtime;
        }
        
        return to_return;
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    get_pwd
     * Type:    Function
     * Purpose: Returns the path to and the name of the present working directory.
     * Input:   Nothing.
     * Output:  string, containing the path to and name of the present working directory.
    ----------------------------------------------------------------------------------------------*/
    string ext2::get_pwd()
    {
        string to_return;
        
        // Check if the pwd is the root directory.
        if (pwd.size() == 1)
        {
            to_return = "/";
        }
        else
        {
            // Otherwise, iterate through the path until the end, adding the name of the directories
            // to the string.
            for (u32 i = 1; i < pwd.size(); i++)
            {
                to_return += "/" + pwd[i].name;
            }
        }
        
        // Return the full path.
        return to_return;
    }

    
    /*----------------------------------------------------------------------------------------------
     * Name:    set_pwd
     * Type:    Function
     * Purpose: Sets the path to and the name of the present working directory.
     * Input:   string, containing the path to and name of the desired working directory.
     * Output:  Nothing.
     *
     * @TODO    Clean out dead code. / Make code look pretty.
    ----------------------------------------------------------------------------------------------*/
    void ext2::set_pwd(const string & desired_pwd)
    {
        // get pwd inode
        // parse directory inode
        // check returned results
        // if the returned results have a match to desired_pwd, change to that directory
        
        // u32 inode = pwd.back().inode;
        // vector<ext2_dir_entry> parsed_dir_inode = parse_directory_inode(inode);
        // for (u32 i = 0; i < parsed_dir_inode.size(); i++)
        // {
        //     if (parsed_dir_inode[i].file_type == 2 && parsed_dir_inode[i].name == desired_pwd)
        //         pwd.push_back(parsed_dir_inode[i]);
        // }
        
        vector<ext2_dir_entry> temp_pwd = dir_entry_exists(desired_pwd);
        if (temp_pwd.size() > 0)
            pwd = temp_pwd;
        
        // vector<string> tokens_desired = utility::tokenize(desired_pwd);
        // string to_return;
        // list<ext2_dir_entry>::iterator it = pwd.begin();
        
        // // Iterate through the path until the end, adding the name of the directories to the string.
        // while (it != pwd.end())
        // {
        //     to_return += "/" + it->name;
        //     it++;
        // }
        
        return;
    }

    
    /*----------------------------------------------------------------------------------------------
     * Name:    file_read
     * Type:    Function
     * Purpose: Reads a file from the virtual filesystem and outputs it on to the host filesystem.
     * Input:   fstream & output_file, contains the output file stream.
     * Input:   const string & file_to_read, holds the name of the file to read from the virtual
     *          filesystem.
     * Output:  bool, representing whether the file was found and read (true) or not (false).
    ----------------------------------------------------------------------------------------------*/
    bool ext2::file_read(fstream & output_file, const string & file_to_read)
    {
        // Check if the file specified in file_to_read exists or not, and if it does, its inode
        // number will be stored in file_inode.
        u32 file_inode = 0;
        if (file_entry_exists(file_to_read, file_inode) == true)
        {
            cout << "debug >> ext2::file_read >> file exists\n";
            
            // Get the file size.
            size_t file_size = readInode(file_inode).i_size;
            
            // Unroll the block chain where the file resides.
            list<u32> file_block_list = make_block_list(file_inode);
            cout << "debug (ext2::file_read) number of blocks: " << file_block_list.size() << endl;
            cin.ignore(1);
            
            // Set up an iterator to go through the block list.
            list<u32>::iterator iter = file_block_list.begin();
            
            // Set up some housekeeping variables.
            size_t bytes_read = 0;
            size_t bytes_to_read = 0;
            
            // Buffer to receive the file contents. Must be char (versus u8) or compiler pukes.
            char * read_buffer = new char[block_size_actual];

            // Loop until the whole file has been read.
            // @TODO determine if a (iter != file_block_list.end()) condition should be added.
            while (bytes_read < file_size)
            {
                // Determine how many bytes to read.  Read a full block if possible, otherwise read
                // just until the end of the file.
                bytes_to_read = (block_size_actual < file_size - bytes_read ? block_size_actual : file_size - bytes_read);
                
                // Set the VDI seek pointer to the apropriate position and then read the designated
                // number of bytes.
                vdi->vdiSeek(blockToOffset(*iter), SEEK_SET);
                vdi->vdiRead(read_buffer, bytes_to_read);
                
                // Immediately write the buffer to file.
                output_file.write(read_buffer, bytes_to_read);
                ///*--------------------------------------------------------------------------------------------------
 * Author:      
 * Date:        2016-07-03
 * Assignment:  Final Project
 * Source File: ext2.cpp
 * Language:    C/C++
 * Course:      Operating Systems
 * Purpose:     Contains the implementation of the ext2 class.
 -------------------------------------------------------------------------------------------------*/

#include "constants.h"
#include "datatypes.h"
#include "ext2.h"
#include "utility.h"
#include "vdi_reader.h"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <ctime>

/*
 * EXT2 should read the MBR, boot sector, super block, and navigate through an EXT2 filesystem.
 */

using namespace std;
using namespace vdi_explorer;

namespace vdi_explorer
{
    /*----------------------------------------------------------------------------------------------
     * Name:    ext2
     * Type:    Function
     * Purpose: Constructor for the ext2 class. Reads in MBR and Superblock.
     * Input:   vdi_reader *_vdi, containing pointer to vdi object.
     * Output:  Nothing.
    ----------------------------------------------------------------------------------------------*/
    ext2::ext2(vdi_reader *_vdi)
    {
        // Store the pointer to the VDI reader and validate it.
        vdi = _vdi;
        if (!vdi)
        {
            cout << "Error opening the vdi reader object.";
            throw;
        }
        
        // Read the MBR boot sector.
        vdi->vdiSeek(0, SEEK_SET);
        vdi->vdiRead(&bootSector, 512);
        
        // Debug info.
        print_bootsector();
        // End debug info.
        
        // Determine the start of the superblock.
        superblock_start = bootSector.partitionTable[0].firstSector * VDI_SECTOR_SIZE +
                           EXT2_SUPERBLOCK_OFFSET;
        
        // Read the superblock.
        vdi->vdiSeek(superblock_start, SEEK_SET);
        vdi->vdiRead(&superblock, sizeof(ext2_superblock));
        
        // Record the actual block size.
        block_size_actual = EXT2_BLOCK_BASE_SIZE << superblock.s_log_block_size;
        
        // Calculate the max allowable file size.
        u16 dwords_per_block = block_size_actual / 4;
        u64 max_file_size_by_block = (dwords_per_block * dwords_per_block * dwords_per_block +
                                      dwords_per_block * dwords_per_block +
                                      dwords_per_block +
                                      EXT2_INODE_NBLOCKS_DIR) *
                                      block_size_actual;
        if (max_file_size_by_block < EXT2_MAX_ABS_FILE_SIZE)
            max_file_size = max_file_size_by_block;
        
        // Debug info.
        print_superblock();
        // End debug info.
        
        // Calculate and verify the number of block groups in the partition.
        u32 nBlockGroupCalc1, nBlockGroupCalc2;
        nBlockGroupCalc1 = (float)superblock.s_blocks_count / superblock.s_blocks_per_group + 0.5;
        nBlockGroupCalc2 = (float)superblock.s_inodes_count / superblock.s_inodes_per_group + 0.5;
        if (nBlockGroupCalc1 != nBlockGroupCalc2)
        {
            cout << "Block group calculation mismatch.";
            throw;
        }
        numBlockGroups = nBlockGroupCalc1;
        
        // Debug info.
        cout << "\n# of Block Groups:\n";
        cout << "Calculation 1: " << nBlockGroupCalc1 << endl;
        cout << "Calculation 2: " << nBlockGroupCalc2 << endl;
        // End debug info.
        
        // Allocate and verify an array of pointers to the (as yes unloaded) block group
        // descriptors.
        bgdTable = new ext2_block_group_desc[numBlockGroups];
        if (bgdTable == nullptr)
        {
            cout << "Error allocating the array of block group descriptors.";
            throw;
        }
        
        // Determine where the start of the block group descriptor table is.
        bgd_table_start = superblock.s_log_block_size == 0 ?
                          blockToOffset(2) :
                          blockToOffset(1);
        
        // Debug info.
        cout << "BGD Table Start: " << bgd_table_start << endl;
        // End debug info.
        
        // Read the block group descriptor table into memory.
        vdi->vdiSeek(bgd_table_start, SEEK_SET);
        vdi->vdiRead(bgdTable, sizeof(ext2_block_group_desc) * numBlockGroups);
        
        // Debug info.
        print_bgd_table();
        // End debug info.
        
        ext2_inode temp = readInode(2);//18);// 30481);
        
        // Debug info.
        print_inode(&temp);
        // End debug info.
        
        // u8 dummy = 0;
        // vdi->vdiSeek(blockToOffset(temp.i_block[0]), SEEK_SET);
        // vdi->vdiRead(&dummy, 1);
        
        auto temp2 = parse_directory_inode(temp);
        for (u32 i = 0; i < temp2.size(); i++)
            print_dir_entry(temp2[i], true);
        
        // set the root folder as the initial pwd.
        // pwd_inode.emplace_back(readInode(2));
        // pwd.emplace_back(parse_directory_inode(pwd_inode.back())[0]);
        pwd.emplace_back(parse_directory_inode(readInode(2))[0]);
        pwd.back().name.assign("/");
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    ~ext2
     * Type:    Function
     * Purpose: Destructor for the ext2 class.
     * Input:   Nothing.
     * Output:  Nothing.
    ----------------------------------------------------------------------------------------------*/
    ext2::~ext2()
    {
        // Delete the block descriptor table.
        if (bgdTable != nullptr)
            delete[] bgdTable;
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    list_directory_contents
     * Type:    Function
     * Purpose: Returns the contents of the current directory.
     * Input:   Nothing.
     * Output:  vector<fs_entry_posix>, containing a listing of all the files in the present working
     *          directory.
     *
     * @TODO    Optimize.
    ----------------------------------------------------------------------------------------------*/
    vector<fs_entry_posix> ext2::get_directory_contents(void)
    {
        vector<fs_entry_posix> to_return;
        ext2_inode temp_inode;
        
        // Parse the contents of the current directory.
        vector<ext2_dir_entry> directory_contents = parse_directory_inode(pwd.back().inode);
        
        // Instead of returning the raw contents, add a layer of abstraction to help facilitate
        // future design plans (eventual support for different filesystems).
        for (u32 i = 0; i < directory_contents.size(); i++)
        {
            // Read the entry's inode to gather data.
            temp_inode = readInode(directory_contents[i].inode);
            
            // Add an fs_entry_posix object to the back of the to_return vector and fill it with the
            // appropriate data.
            to_return.emplace_back();
            to_return.back().name = directory_contents[i].name;
            to_return.back().type = directory_contents[i].file_type;
            to_return.back().permissions = temp_inode.i_mode & 0x0fff; // mask for the bottom 12 bits
            to_return.back().user_id = temp_inode.i_uid;
            to_return.back().group_id = temp_inode.i_gid;
            to_return.back().size = temp_inode.i_size;
            to_return.back().timestamp_created = temp_inode.i_ctime;
            to_return.back().timestamp_modified = temp_inode.i_mtime;
        }
        
        return to_return;
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    get_pwd
     * Type:    Function
     * Purpose: Returns the path to and the name of the present working directory.
     * Input:   Nothing.
     * Output:  string, containing the path to and name of the present working directory.
    ----------------------------------------------------------------------------------------------*/
    string ext2::get_pwd()
    {
        string to_return;
        
        // Check if the pwd is the root directory.
        if (pwd.size() == 1)
        {
            to_return = "/";
        }
        else
        {
            // Otherwise, iterate through the path until the end, adding the name of the directories
            // to the string.
            for (u32 i = 1; i < pwd.size(); i++)
            {
                to_return += "/" + pwd[i].name;
            }
        }
        
        // Return the full path.
        return to_return;
    }

    
    /*----------------------------------------------------------------------------------------------
     * Name:    set_pwd
     * Type:    Function
     * Purpose: Sets the path to and the name of the present working directory.
     * Input:   string, containing the path to and name of the desired working directory.
     * Output:  Nothing.
     *
     * @TODO    Clean out dead code. / Make code look pretty.
    ----------------------------------------------------------------------------------------------*/
    void ext2::set_pwd(const string & desired_pwd)
    {
        // get pwd inode
        // parse directory inode
        // check returned results
        // if the returned results have a match to desired_pwd, change to that directory
        
        // u32 inode = pwd.back().inode;
        // vector<ext2_dir_entry> parsed_dir_inode = parse_directory_inode(inode);
        // for (u32 i = 0; i < parsed_dir_inode.size(); i++)
        // {
        //     if (parsed_dir_inode[i].file_type == 2 && parsed_dir_inode[i].name == desired_pwd)
        //         pwd.push_back(parsed_dir_inode[i]);
        // }
        
        vector<ext2_dir_entry> temp_pwd = dir_entry_exists(desired_pwd);
        if (temp_pwd.size() > 0)
            pwd = temp_pwd;
        
        // vector<string> tokens_desired = utility::tokenize(desired_pwd);
        // string to_return;
        // list<ext2_dir_entry>::iterator it = pwd.begin();
        
        // // Iterate through the path until the end, adding the name of the directories to the string.
        // while (it != pwd.end())
        // {
        //     to_return += "/" + it->name;
        //     it++;
        // }
        
        return;
    }

    
    /*----------------------------------------------------------------------------------------------
     * Name:    file_read
     * Type:    Function
     * Purpose: Reads a file from the virtual filesystem and outputs it on to the host filesystem.
     * Input:   fstream & output_file, contains the output file stream.
     * Input:   const string & file_to_read, holds the name of the file to read from the virtual
     *          filesystem.
     * Output:  bool, representing whether the file was found and read (true) or not (false).
    ----------------------------------------------------------------------------------------------*/
    bool ext2::file_read(fstream & output_file, const string & file_to_read)
    {
        // Check if the file specified in file_to_read exists or not, and if it does, its inode
        // number will be stored in file_inode.
        u32 file_inode = 0;
        if (file_entry_exists(file_to_read, file_inode) == true)
        {
            cout << "debug >> ext2::file_read >> file exists\n";
            
            // Get the file size.
            size_t file_size = readInode(file_inode).i_size;
            
            // Unroll the block chain where the file resides.
            list<u32> file_block_list = make_block_list(file_inode);
            cout << "debug (ext2::file_read) number of blocks: " << file_block_list.size() << endl;
            cin.ignore(1);
            
            // Set up an iterator to go through the block list.
            list<u32>::iterator iter = file_block_list.begin();
            
            // Set up some housekeeping variables.
            size_t bytes_read = 0;
            size_t bytes_to_read = 0;
            
            // Buffer to receive the file contents. Must be char (versus u8) or compiler pukes.
            char * read_buffer = new char[block_size_actual];

            // Loop until the whole file has been read.
            // @TODO determine if a (iter != file_block_list.end()) condition should be added.
            while (bytes_read < file_size)
            {
                // Determine how many bytes to read.  Read a full block if possible, otherwise read
                // just until the end of the file.
                bytes_to_read = (block_size_actual < file_size - bytes_read ? block_size_actual : file_size - bytes_read);
                
                // Set the VDI seek pointer to the apropriate position and then read the designated
                // number of bytes.
                vdi->vdiSeek(blockToOffset(*iter), SEEK_SET);
                vdi->vdiRead(read_buffer, bytes_to_read);
                
                // Immediately write the buffer to file.
                output_file.write(read_buffer, bytes_to_read);
                ///*--------------------------------------------------------------------------------------------------
 * Author:      
 * Date:        2016-07-03
 * Assignment:  Final Project
 * Source File: ext2.cpp
 * Language:    C/C++
 * Course:      Operating Systems
 * Purpose:     Contains the implementation of the ext2 class.
 -------------------------------------------------------------------------------------------------*/

#include "constants.h"
#include "datatypes.h"
#include "ext2.h"
#include "utility.h"
#include "vdi_reader.h"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <ctime>

/*
 * EXT2 should read the MBR, boot sector, super block, and navigate through an EXT2 filesystem.
 */

using namespace std;
using namespace vdi_explorer;

namespace vdi_explorer
{
    /*----------------------------------------------------------------------------------------------
     * Name:    ext2
     * Type:    Function
     * Purpose: Constructor for the ext2 class. Reads in MBR and Superblock.
     * Input:   vdi_reader *_vdi, containing pointer to vdi object.
     * Output:  Nothing.
    ----------------------------------------------------------------------------------------------*/
    ext2::ext2(vdi_reader *_vdi)
    {
        // Store the pointer to the VDI reader and validate it.
        vdi = _vdi;
        if (!vdi)
        {
            cout << "Error opening the vdi reader object.";
            throw;
        }
        
        // Read the MBR boot sector.
        vdi->vdiSeek(0, SEEK_SET);
        vdi->vdiRead(&bootSector, 512);
        
        // Debug info.
        print_bootsector();
        // End debug info.
        
        // Determine the start of the superblock.
        superblock_start = bootSector.partitionTable[0].firstSector * VDI_SECTOR_SIZE +
                           EXT2_SUPERBLOCK_OFFSET;
        
        // Read the superblock.
        vdi->vdiSeek(superblock_start, SEEK_SET);
        vdi->vdiRead(&superblock, sizeof(ext2_superblock));
        
        // Record the actual block size.
        block_size_actual = EXT2_BLOCK_BASE_SIZE << superblock.s_log_block_size;
        
        // Calculate the max allowable file size.
        u16 dwords_per_block = block_size_actual / 4;
        u64 max_file_size_by_block = (dwords_per_block * dwords_per_block * dwords_per_block +
                                      dwords_per_block * dwords_per_block +
                                      dwords_per_block +
                                      EXT2_INODE_NBLOCKS_DIR) *
                                      block_size_actual;
        if (max_file_size_by_block < EXT2_MAX_ABS_FILE_SIZE)
            max_file_size = max_file_size_by_block;
        
        // Debug info.
        print_superblock();
        // End debug info.
        
        // Calculate and verify the number of block groups in the partition.
        u32 nBlockGroupCalc1, nBlockGroupCalc2;
        nBlockGroupCalc1 = (float)superblock.s_blocks_count / superblock.s_blocks_per_group + 0.5;
        nBlockGroupCalc2 = (float)superblock.s_inodes_count / superblock.s_inodes_per_group + 0.5;
        if (nBlockGroupCalc1 != nBlockGroupCalc2)
        {
            cout << "Block group calculation mismatch.";
            throw;
        }
        numBlockGroups = nBlockGroupCalc1;
        
        // Debug info.
        cout << "\n# of Block Groups:\n";
        cout << "Calculation 1: " << nBlockGroupCalc1 << endl;
        cout << "Calculation 2: " << nBlockGroupCalc2 << endl;
        // End debug info.
        
        // Allocate and verify an array of pointers to the (as yes unloaded) block group
        // descriptors.
        bgdTable = new ext2_block_group_desc[numBlockGroups];
        if (bgdTable == nullptr)
        {
            cout << "Error allocating the array of block group descriptors.";
            throw;
        }
        
        // Determine where the start of the block group descriptor table is.
        bgd_table_start = superblock.s_log_block_size == 0 ?
                          blockToOffset(2) :
                          blockToOffset(1);
        
        // Debug info.
        cout << "BGD Table Start: " << bgd_table_start << endl;
        // End debug info.
        
        // Read the block group descriptor table into memory.
        vdi->vdiSeek(bgd_table_start, SEEK_SET);
        vdi->vdiRead(bgdTable, sizeof(ext2_block_group_desc) * numBlockGroups);
        
        // Debug info.
        print_bgd_table();
        // End debug info.
        
        ext2_inode temp = readInode(2);//18);// 30481);
        
        // Debug info.
        print_inode(&temp);
        // End debug info.
        
        // u8 dummy = 0;
        // vdi->vdiSeek(blockToOffset(temp.i_block[0]), SEEK_SET);
        // vdi->vdiRead(&dummy, 1);
        
        auto temp2 = parse_directory_inode(temp);
        for (u32 i = 0; i < temp2.size(); i++)
            print_dir_entry(temp2[i], true);
        
        // set the root folder as the initial pwd.
        // pwd_inode.emplace_back(readInode(2));
        // pwd.emplace_back(parse_directory_inode(pwd_inode.back())[0]);
        pwd.emplace_back(parse_directory_inode(readInode(2))[0]);
        pwd.back().name.assign("/");
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    ~ext2
     * Type:    Function
     * Purpose: Destructor for the ext2 class.
     * Input:   Nothing.
     * Output:  Nothing.
    ----------------------------------------------------------------------------------------------*/
    ext2::~ext2()
    {
        // Delete the block descriptor table.
        if (bgdTable != nullptr)
            delete[] bgdTable;
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    list_directory_contents
     * Type:    Function
     * Purpose: Returns the contents of the current directory.
     * Input:   Nothing.
     * Output:  vector<fs_entry_posix>, containing a listing of all the files in the present working
     *          directory.
     *
     * @TODO    Optimize.
    ----------------------------------------------------------------------------------------------*/
    vector<fs_entry_posix> ext2::get_directory_contents(void)
    {
        vector<fs_entry_posix> to_return;
        ext2_inode temp_inode;
        
        // Parse the contents of the current directory.
        vector<ext2_dir_entry> directory_contents = parse_directory_inode(pwd.back().inode);
        
        // Instead of returning the raw contents, add a layer of abstraction to help facilitate
        // future design plans (eventual support for different filesystems).
        for (u32 i = 0; i < directory_contents.size(); i++)
        {
            // Read the entry's inode to gather data.
            temp_inode = readInode(directory_contents[i].inode);
            
            // Add an fs_entry_posix object to the back of the to_return vector and fill it with the
            // appropriate data.
            to_return.emplace_back();
            to_return.back().name = directory_contents[i].name;
            to_return.back().type = directory_contents[i].file_type;
            to_return.back().permissions = temp_inode.i_mode & 0x0fff; // mask for the bottom 12 bits
            to_return.back().user_id = temp_inode.i_uid;
            to_return.back().group_id = temp_inode.i_gid;
            to_return.back().size = temp_inode.i_size;
            to_return.back().timestamp_created = temp_inode.i_ctime;
            to_return.back().timestamp_modified = temp_inode.i_mtime;
        }
        
        return to_return;
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    get_pwd
     * Type:    Function
     * Purpose: Returns the path to and the name of the present working directory.
     * Input:   Nothing.
     * Output:  string, containing the path to and name of the present working directory.
    ----------------------------------------------------------------------------------------------*/
    string ext2::get_pwd()
    {
        string to_return;
        
        // Check if the pwd is the root directory.
        if (pwd.size() == 1)
        {
            to_return = "/";
        }
        else
        {
            // Otherwise, iterate through the path until the end, adding the name of the directories
            // to the string.
            for (u32 i = 1; i < pwd.size(); i++)
            {
                to_return += "/" + pwd[i].name;
            }
        }
        
        // Return the full path.
        return to_return;
    }

    
    /*----------------------------------------------------------------------------------------------
     * Name:    set_pwd
     * Type:    Function
     * Purpose: Sets the path to and the name of the present working directory.
     * Input:   string, containing the path to and name of the desired working directory.
     * Output:  Nothing.
     *
     * @TODO    Clean out dead code. / Make code look pretty.
    ----------------------------------------------------------------------------------------------*/
    void ext2::set_pwd(const string & desired_pwd)
    {
        // get pwd inode
        // parse directory inode
        // check returned results
        // if the returned results have a match to desired_pwd, change to that directory
        
        // u32 inode = pwd.back().inode;
        // vector<ext2_dir_entry> parsed_dir_inode = parse_directory_inode(inode);
        // for (u32 i = 0; i < parsed_dir_inode.size(); i++)
        // {
        //     if (parsed_dir_inode[i].file_type == 2 && parsed_dir_inode[i].name == desired_pwd)
        //         pwd.push_back(parsed_dir_inode[i]);
        // }
        
        vector<ext2_dir_entry> temp_pwd = dir_entry_exists(desired_pwd);
        if (temp_pwd.size() > 0)
            pwd = temp_pwd;
        
        // vector<string> tokens_desired = utility::tokenize(desired_pwd);
        // string to_return;
        // list<ext2_dir_entry>::iterator it = pwd.begin();
        
        // // Iterate through the path until the end, adding the name of the directories to the string.
        // while (it != pwd.end())
        // {
        //     to_return += "/" + it->name;
        //     it++;
        // }
        
        return;
    }

    
    /*----------------------------------------------------------------------------------------------
     * Name:    file_read
     * Type:    Function
     * Purpose: Reads a file from the virtual filesystem and outputs it on to the host filesystem.
     * Input:   fstream & output_file, contains the output file stream.
     * Input:   const string & file_to_read, holds the name of the file to read from the virtual
     *          filesystem.
     * Output:  bool, representing whether the file was found and read (true) or not (false).
    ----------------------------------------------------------------------------------------------*/
    bool ext2::file_read(fstream & output_file, const string & file_to_read)
    {
        // Check if the file specified in file_to_read exists or not, and if it does, its inode
        // number will be stored in file_inode.
        u32 file_inode = 0;
        if (file_entry_exists(file_to_read, file_inode) == true)
        {
            cout << "debug >> ext2::file_read >> file exists\n";
            
            // Get the file size.
            size_t file_size = readInode(file_inode).i_size;
            
            // Unroll the block chain where the file resides.
            list<u32> file_block_list = make_block_list(file_inode);
            cout << "debug (ext2::file_read) number of blocks: " << file_block_list.size() << endl;
            cin.ignore(1);
            
            // Set up an iterator to go through the block list.
            list<u32>::iterator iter = file_block_list.begin();
            
            // Set up some housekeeping variables.
            size_t bytes_read = 0;
            size_t bytes_to_read = 0;
            
            // Buffer to receive the file contents. Must be char (versus u8) or compiler pukes.
            char * read_buffer = new char[block_size_actual];

            // Loop until the whole file has been read.
            // @TODO determine if a (iter != file_block_list.end()) condition should be added.
            while (bytes_read < file_size)
            {
                // Determine how many bytes to read.  Read a full block if possible, otherwise read
                // just until the end of the file.
                bytes_to_read = (block_size_actual < file_size - bytes_read ? block_size_actual : file_size - bytes_read);
                
                // Set the VDI seek pointer to the apropriate position and then read the designated
                // number of bytes.
                vdi->vdiSeek(blockToOffset(*iter), SEEK_SET);
                vdi->vdiRead(read_buffer, bytes_to_read);
                
                // Immediately write the buffer to file.
                output_file.write(read_buffer, bytes_to_read);
                ///*--------------------------------------------------------------------------------------------------
 * Author:      
 * Date:        2016-07-03
 * Assignment:  Final Project
 * Source File: ext2.cpp
 * Language:    C/C++
 * Course:      Operating Systems
 * Purpose:     Contains the implementation of the ext2 class.
 -------------------------------------------------------------------------------------------------*/

#include "constants.h"
#include "datatypes.h"
#include "ext2.h"
#include "utility.h"
#include "vdi_reader.h"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <ctime>

/*
 * EXT2 should read the MBR, boot sector, super block, and navigate through an EXT2 filesystem.
 */

using namespace std;
using namespace vdi_explorer;

namespace vdi_explorer
{
    /*----------------------------------------------------------------------------------------------
     * Name:    ext2
     * Type:    Function
     * Purpose: Constructor for the ext2 class. Reads in MBR and Superblock.
     * Input:   vdi_reader *_vdi, containing pointer to vdi object.
     * Output:  Nothing.
    ----------------------------------------------------------------------------------------------*/
    ext2::ext2(vdi_reader *_vdi)
    {
        // Store the pointer to the VDI reader and validate it.
        vdi = _vdi;
        if (!vdi)
        {
            cout << "Error opening the vdi reader object.";
            throw;
        }
        
        // Read the MBR boot sector.
        vdi->vdiSeek(0, SEEK_SET);
        vdi->vdiRead(&bootSector, 512);
        
        // Debug info.
        print_bootsector();
        // End debug info.
        
        // Determine the start of the superblock.
        superblock_start = bootSector.partitionTable[0].firstSector * VDI_SECTOR_SIZE +
                           EXT2_SUPERBLOCK_OFFSET;
        
        // Read the superblock.
        vdi->vdiSeek(superblock_start, SEEK_SET);
        vdi->vdiRead(&superblock, sizeof(ext2_superblock));
        
        // Record the actual block size.
        block_size_actual = EXT2_BLOCK_BASE_SIZE << superblock.s_log_block_size;
        
        // Calculate the max allowable file size.
        u16 dwords_per_block = block_size_actual / 4;
        u64 max_file_size_by_block = (dwords_per_block * dwords_per_block * dwords_per_block +
                                      dwords_per_block * dwords_per_block +
                                      dwords_per_block +
                                      EXT2_INODE_NBLOCKS_DIR) *
                                      block_size_actual;
        if (max_file_size_by_block < EXT2_MAX_ABS_FILE_SIZE)
            max_file_size = max_file_size_by_block;
        
        // Debug info.
        print_superblock();
        // End debug info.
        
        // Calculate and verify the number of block groups in the partition.
        u32 nBlockGroupCalc1, nBlockGroupCalc2;
        nBlockGroupCalc1 = (float)superblock.s_blocks_count / superblock.s_blocks_per_group + 0.5;
        nBlockGroupCalc2 = (float)superblock.s_inodes_count / superblock.s_inodes_per_group + 0.5;
        if (nBlockGroupCalc1 != nBlockGroupCalc2)
        {
            cout << "Block group calculation mismatch.";
            throw;
        }
        numBlockGroups = nBlockGroupCalc1;
        
        // Debug info.
        cout << "\n# of Block Groups:\n";
        cout << "Calculation 1: " << nBlockGroupCalc1 << endl;
        cout << "Calculation 2: " << nBlockGroupCalc2 << endl;
        // End debug info.
        
        // Allocate and verify an array of pointers to the (as yes unloaded) block group
        // descriptors.
        bgdTable = new ext2_block_group_desc[numBlockGroups];
        if (bgdTable == nullptr)
        {
            cout << "Error allocating the array of block group descriptors.";
            throw;
        }
        
        // Determine where the start of the block group descriptor table is.
        bgd_table_start = superblock.s_log_block_size == 0 ?
                          blockToOffset(2) :
                          blockToOffset(1);
        
        // Debug info.
        cout << "BGD Table Start: " << bgd_table_start << endl;
        // End debug info.
        
        // Read the block group descriptor table into memory.
        vdi->vdiSeek(bgd_table_start, SEEK_SET);
        vdi->vdiRead(bgdTable, sizeof(ext2_block_group_desc) * numBlockGroups);
        
        // Debug info.
        print_bgd_table();
        // End debug info.
        
        ext2_inode temp = readInode(2);//18);// 30481);
        
        // Debug info.
        print_inode(&temp);
        // End debug info.
        
        // u8 dummy = 0;
        // vdi->vdiSeek(blockToOffset(temp.i_block[0]), SEEK_SET);
        // vdi->vdiRead(&dummy, 1);
        
        auto temp2 = parse_directory_inode(temp);
        for (u32 i = 0; i < temp2.size(); i++)
            print_dir_entry(temp2[i], true);
        
        // set the root folder as the initial pwd.
        // pwd_inode.emplace_back(readInode(2));
        // pwd.emplace_back(parse_directory_inode(pwd_inode.back())[0]);
        pwd.emplace_back(parse_directory_inode(readInode(2))[0]);
        pwd.back().name.assign("/");
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    ~ext2
     * Type:    Function
     * Purpose: Destructor for the ext2 class.
     * Input:   Nothing.
     * Output:  Nothing.
    ----------------------------------------------------------------------------------------------*/
    ext2::~ext2()
    {
        // Delete the block descriptor table.
        if (bgdTable != nullptr)
            delete[] bgdTable;
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    list_directory_contents
     * Type:    Function
     * Purpose: Returns the contents of the current directory.
     * Input:   Nothing.
     * Output:  vector<fs_entry_posix>, containing a listing of all the files in the present working
     *          directory.
     *
     * @TODO    Optimize.
    ----------------------------------------------------------------------------------------------*/
    vector<fs_entry_posix> ext2::get_directory_contents(void)
    {
        vector<fs_entry_posix> to_return;
        ext2_inode temp_inode;
        
        // Parse the contents of the current directory.
        vector<ext2_dir_entry> directory_contents = parse_directory_inode(pwd.back().inode);
        
        // Instead of returning the raw contents, add a layer of abstraction to help facilitate
        // future design plans (eventual support for different filesystems).
        for (u32 i = 0; i < directory_contents.size(); i++)
        {
            // Read the entry's inode to gather data.
            temp_inode = readInode(directory_contents[i].inode);
            
            // Add an fs_entry_posix object to the back of the to_return vector and fill it with the
            // appropriate data.
            to_return.emplace_back();
            to_return.back().name = directory_contents[i].name;
            to_return.back().type = directory_contents[i].file_type;
            to_return.back().permissions = temp_inode.i_mode & 0x0fff; // mask for the bottom 12 bits
            to_return.back().user_id = temp_inode.i_uid;
            to_return.back().group_id = temp_inode.i_gid;
            to_return.back().size = temp_inode.i_size;
            to_return.back().timestamp_created = temp_inode.i_ctime;
            to_return.back().timestamp_modified = temp_inode.i_mtime;
        }
        
        return to_return;
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    get_pwd
     * Type:    Function
     * Purpose: Returns the path to and the name of the present working directory.
     * Input:   Nothing.
     * Output:  string, containing the path to and name of the present working directory.
    ----------------------------------------------------------------------------------------------*/
    string ext2::get_pwd()
    {
        string to_return;
        
        // Check if the pwd is the root directory.
        if (pwd.size() == 1)
        {
            to_return = "/";
        }
        else
        {
            // Otherwise, iterate through the path until the end, adding the name of the directories
            // to the string.
            for (u32 i = 1; i < pwd.size(); i++)
            {
                to_return += "/" + pwd[i].name;
            }
        }
        
        // Return the full path.
        return to_return;
    }

    
    /*----------------------------------------------------------------------------------------------
     * Name:    set_pwd
     * Type:    Function
     * Purpose: Sets the path to and the name of the present working directory.
     * Input:   string, containing the path to and name of the desired working directory.
     * Output:  Nothing.
     *
     * @TODO    Clean out dead code. / Make code look pretty.
    ----------------------------------------------------------------------------------------------*/
    void ext2::set_pwd(const string & desired_pwd)
    {
        // get pwd inode
        // parse directory inode
        // check returned results
        // if the returned results have a match to desired_pwd, change to that directory
        
        // u32 inode = pwd.back().inode;
        // vector<ext2_dir_entry> parsed_dir_inode = parse_directory_inode(inode);
        // for (u32 i = 0; i < parsed_dir_inode.size(); i++)
        // {
        //     if (parsed_dir_inode[i].file_type == 2 && parsed_dir_inode[i].name == desired_pwd)
        //         pwd.push_back(parsed_dir_inode[i]);
        // }
        
        vector<ext2_dir_entry> temp_pwd = dir_entry_exists(desired_pwd);
        if (temp_pwd.size() > 0)
            pwd = temp_pwd;
        
        // vector<string> tokens_desired = utility::tokenize(desired_pwd);
        // string to_return;
        // list<ext2_dir_entry>::iterator it = pwd.begin();
        
        // // Iterate through the path until the end, adding the name of the directories to the string.
        // while (it != pwd.end())
        // {
        //     to_return += "/" + it->name;
        //     it++;
        // }
        
        return;
    }

    
    /*----------------------------------------------------------------------------------------------
     * Name:    file_read
     * Type:    Function
     * Purpose: Reads a file from the virtual filesystem and outputs it on to the host filesystem.
     * Input:   fstream & output_file, contains the output file stream.
     * Input:   const string & file_to_read, holds the name of the file to read fr