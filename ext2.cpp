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
                           EXT2_SUPERBLOCK_OFFSET
        
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
                //output_file.
                
                // Get set up to do the next read by setting the iterated to the next block in the
                // chain and incrementing the number of bytes that have been read.
                iter++;
                bytes_read += bytes_to_read;
            }
            
            // Release the read buffer's memory and return.
            delete[] read_buffer;
            return true;
        }
        else
        {
            cout << "debug >> ext2::file_read >> file does not exist\n";
            // The file does not exist, so return false.
            return false;
        }
    }


    /*----------------------------------------------------------------------------------------------
     * Name:    file_write
     * Type:    Function
     * Purpose: Writes a file from the host filesystem and into the filesystem being accessed by
     *          this program.
     * Input:   fstream & input_file, contains the input file stream.
     * Input:   string filename_to_write, holds the name of the file to write to in the filesystem.
     * Input:   const u32 dir_inode_num, holds the inode number of the directory inode where the
     *          file's information will be added.
     * Output:  bool, representing whether the file was written (true) or not (false).
    ----------------------------------------------------------------------------------------------*/
    bool ext2::file_write(fstream & input_file, string filename_to_write, const u32 dir_inode_num)
    {
        // Tasks to complete:
        //   [x] check if file already exists
        //   [x] verify inode is valid
        //   [x] determine input file size
        //   [x] make sure the file is under the max size
        //   [x] truncate filename_to_write if needed (max size is EXT2_FILENAME_MAX_LENGTH)
        //   [x] determine how many blocks the file is going to take including supporting
        //       structures, such as inode and indirect blocks, and check directory inode to make
        //       sure it can handle another ext2_dir_entry (or if it needs another block to handle
        //       it)
        //   [x] verify that there are enough free blocks
        //   [x] make a list of free blocks that we plan on using (try to use the ones that are in
        //       the same block group as the parent directly)
        //   [x] write incoming file to disk
        //   [x] record blocks in indirect block structures if necessary
        //   [x] find free inode
        //   [x] build inode entry (don't forget about permissions!)
        //   [x] write inode entry to disk
        //   [ ] add ext2_dir_entry to directory block
        //   [x] modify block group descriptor block bitmaps on disk
        //   [x] modify block group descriptor inode bitmap on disk
        //   [x] modify block group descriptor table in memory
        //   [x] modify block group descriptor table on disk
        //   [x] modify superblock in memory
        //   [x] modify superblock on disk
        //
        //   Double-check the filesystem documentation to make damn sure we don't overwrite or miss
        //   something critical.
        
        
        /***   Check to see if the file already exists.   ***/
        if (file_entry_exists(filename_to_write, u32 dont_care))
        {
            cout << "Error: File already exists.  (ext2::file_write)\n";
            return false;
        }
        /***   End check to see if the file already exists.   ***/
        
        
        /***   Verify inode is valid.   ***/
        if (dir_inode_num > superblock.s_inodes_count || dir_inode_num < superblock.s_first_ino)
        {
            cout << "Error: Invalid inode number. (ext2::file_write)\n";
            throw;
        }
        /***   End verify inode is valid.   ***/
        
        
        /***   Determine file size.   ***/
        // Assume that the file stream is at the beginning and record the position of the current
        // character in the input stream.
        std::streampos stream_size = input_file.tellg();
        
        // Seek to the end of the file.
        input_file.seekg(0, std::ios::end);
        
        // Record the actual size as the end position minus the beginning position.
        stream_size = input_file.tellg() - stream_size;
        
        // Seek back to the beginning of the file.
        input_file.seekg(0, std::ios::beg);
        
        // Convert the stream size into file size.
        size_t file_size = (u64)stream_size;
        /***   End determine file size.   ***/
        
        
        /***   Ensure the file is under the max size.   ***/
        if (file_size > max_file_size)
        {
            cout << "Error: File is too large to be handled by the file system. (ext2::file_write)\n";
            return false;
        }
        /***   End ensure the file is under the max size.   ***/
        
        
        /***   Truncate filename_to_write to EXT2_FILENAME_MAX_LENGTH bytes if needed.   ***/
        if (filename_to_write.length() > EXT2_FILENAME_MAX_LENGTH)
        {
            cout << "Warning: File name is too long.  Truncating to " << EXT2_FILENAME_MAX_LENGTH <<
                    " characters.\n";
            filename_to_write.erase(EXT2_FILENAME_MAX_LENGTH);
        }
        /***   End truncate filename_to_write to EXT2_FILENAME_MAX_LENGTH bytes if needed.   ***/
        
        
        /***   Determine how many blocks the file will take up.   ***/
        // NOTE: This section may be more useful as a function.
        // @TODO verify mathy things
        // @TODO optimize, maybe even rework the whole algorithm
        
        // The file will use at least file_size / block_size_actual number of blocks.  Because this
        // uses integer division, the decimal is cut off, hence we use modulus to determine if the
        // block count needs to be expanded by one to compensate. A.k.a poor man's ceiling function.
        u32 raw_num_blocks_needed = (file_size % block_size_actual ?
                                     file_size / block_size_actual + 1 :
                                     file_size / block_size_actual);
        
        // Housekeeping variables to help keep calculations from being as stupidly long.
        u32 s_num_block_pointers = block_size_actual / EXT2_BLOCK_POINTER_SIZE;
        u32 d_num_block_pointers = s_num_block_pointers * s_num_block_pointers;
        
        // Establish a counter for the total number of blocks that will be needed when all support
        // structures, such as indirect blocks, are taken into account.
        u32 total_num_blocks_needed = raw_num_blocks_needed;
        
        // Establish a counter for the number supporting blocks needed (indirect blocks, etc.)
        u32 supporting_blocks_needed = 0;
        
        // Check to see if the singly indirect block will be needed.
        if (raw_num_blocks_needed > EXT2_INODE_NBLOCKS_DIR)
        {
            // Add 1 for the singly indirect block.
            total_num_blocks_needed += 1;
            supporting_blocks_needed += 1;
            
            // Keep track of how many more blocks are needed at each stage.  At this stage,
            // compensate for the number of direct block pointers.
            u32 num_blocks_left = raw_num_blocks_needed - EXT2_INODE_NBLOCKS_DIR;
            
            // Check to see if the doubly indirect block will be needed.
            if (raw_num_blocks_needed > EXT2_INODE_NBLOCKS_DIR + s_num_block_pointers)
            {
                // Add 1 for the doubly indirect block.
                total_num_blocks_needed += 1;
                supporting_blocks_needed += 1;
                
                // Keep track of how many more blocks are needed at each stage.  At this stage,
                // compensate for the number of block pointers contained in the singly indirect
                // block.
                num_blocks_left -= s_num_block_pointers;
                
                // Calculate the number of singly indirect blocks still needed, compensating for
                // integer division.
                u32 num_s_blocks_left = (num_blocks_left % s_num_block_pointers ?
                                         num_blocks_left / s_num_block_pointers + 1 :
                                         num_blocks_left / s_num_block_pointers);
                
                // Whether or not a triply indirect block is needed, this number of singly indirect
                // blocks will be, so add the number of singly indirect blocks left to the total.
                total_num_blocks_needed += num_s_blocks_left;
                supporting_blocks_needed += num_s_blocks_left;
                
                // Check to see if the triply indirect block will be needed.
                if (raw_num_blocks_needed > EXT2_INODE_NBLOCKS_DIR + s_num_block_pointers + 
                                            d_num_block_pointers)
                {
                    // Add 1 for the triply indirect block.
                    total_num_blocks_needed += 1;
                    supporting_blocks_needed += 1;
                    
                    // Keep track of how many more blocks are needed at each stage.  At this stage,
                    // compensate for the number of block pointers contained in the doubly indirect
                    // block.
                    num_blocks_left -= d_num_block_pointers;
                    
                    // Calculate the number of doubly indirect blocks still needed, compensating for
                    // integer division.
                    u32 num_d_blocks_left = (num_blocks_left % d_num_block_pointers ?
                                             num_blocks_left / d_num_block_pointers + 1 :
                                             num_blocks_left / d_num_block_pointers);
                    
                    // Add the number of doubly indirect blocks needed to the total.
                    total_num_blocks_needed += num_d_blocks_left;
                    supporting_blocks_needed += num_d_blocks_left;
                }
            }
        }
        
        // Check to see if the directory inode will need another block to handle the added
        // ext2_dir_entry structure.
        // Establish a variable for downstream subfunctions to easily determine whether a block
        // needs to be added.
        bool additional_dir_block_needed = false;
        
        // Parse the directory inode where the file will live.
        vector<ext2::ext2_dir_entry> directory_entries = parse_directory_inode(dir_inode_num);
        
        // Calculate the number of bytes left in the directory block.
        size_t dir_block_space_left = directory_entries.back().rec_len - 
                                      (EXT2_BLOCK_BASE_SIZE + directory_entries.back().name_len) % 4;
        
        // Calculate the number of bytes needed to store an ext2_dir_entry structure for the file.
        size_t dir_block_space_needed = EXT2_BLOCK_BASE_SIZE + filename_to_write.length();
        dir_block_space_needed += dir_block_space_needed % 4;
        
        // If the number of bytes needed exceeds those available, add 1 to the total number of
        // needed blocks to account for the extra block needed for the directory entry.
        if (dir_block_space_needed > dir_block_space_left)
        {
            additional_dir_block_needed = true;
            total_num_blocks_needed += 1;
            supporting_blocks_needed += 1;
        }
        /***   End determine how many blocks the file will take up.   ***/
        
        
        /***   Verify that enough free space exists.   ***/
        // Check the number of free blocks.
        if (superblock.s_free_blocks_count < total_num_blocks_needed)
        {
            cout << "Error: Not enough free blocks available on the file system.\n";
            throw;
        }
        
        // Check the number of free inodes.
        if (superblock.s_free_inodes_count < 1)
        {
            cout << "Error: Not enough free inodes available on the file system.\n";
            throw;
        }
        /***   End verify that enough free space exists.   ***/
        
        
        /***   Make a list of free blocks we plan on using.   ***/
        // Implement functions specifically for reading / writing bitmaps?
        // What about using vector<bool>?
        //
        // algorithm
        //
        // check whether file will fit into one block group
        // yes:
        //   read just that bdg bitmap
        //   run through bitmap starting at index [2 + (superblock.s_inodes_per_group * superblock.s_inode_size) / block_size_actual]
        //   add every free block up to the number of blocks we need.
        // no:
        //   read all bgd bitmaps
        //   run through each bitmap starting at index starting at index [2 + (superblock.s_inodes_per_group * superblock.s_inode_size) / block_size_actual]
        //   add every block from one block group before moving to the next one, up to the number of blocks we need (probably not good practice... fix at a later date)
        
        // Create a vector to hold the block numbers that will be written.
        vector<u32> blocks_to_write;
        
        // Determine what block group the directory inode is in.
        u32 dir_inode_block_group_num = inodeToBlockGroup(dir_inode_num);
        
        // @TODO Decide if this should simply be read in during object construction.
        vector<vector<bool>> block_group_block_bitmaps; // vector-inception...
        
        // Establish a vector to keep track of "dirty" block group block bitmaps that will need to
        // be written to disk.
        vector<bool> dirty_block_group_block_bitmaps;
        
        // Establish a vector to keep track of how many blocks are being used from each block group.
        vector<u32> num_blocks_used_per_block_group;
        
        // Add enough elements to the vectors to cover the number of block groups.
        for (u32 i = 0; i < numBlockGroups; i++)
        {
            dirty_block_group_block_bitmaps.push_back(0);
            num_blocks_used_per_block_group.push_back(0);
        }
        
        // Housekeeping variable to establish the bitmap index where the real data blocks begin.
        u64 starting_data_block_offset = 1 + // 1 for the block usage bitmap
                                         1 + // 1 for the inode usage bitmap
                                         (superblock.s_inodes_per_group * superblock.s_inode_size) / block_size_actual;
        
        // Check if the block group has enough free blocks to hold the file or if it will have to
        // be written across multiple block groups.
        if (total_num_blocks_needed < bgdTable[dir_inode_block_group_num].bg_free_blocks_count)
        {
            // Only have to read the one bitmap.
            block_group_block_bitmaps.push_back(read_bitmap(bgdTable[dir_inode_block_group_num].bg_block_bitmap, superblock.s_blocks_per_group));
            
            // Iterate through the block group's block bitmap.
            for (u64 i = starting_data_block_offset; i < block_group_block_bitmaps.back().size(); i++)
            {
                // Check that the block is not in use.
                if (block_group_block_bitmaps.back()[i] == false)
                {
                    // Mark the block as in use.
                    block_group_block_bitmaps.back()[i] = true;
                    
                    // Mark the bitmap dirty.
                    dirty_block_group_block_bitmaps[dir_inode_block_group_num] = true;
                    
                    // Increment the number of used blocks in this block group.
                    num_blocks_used_per_block_group[dir_inode_block_group_num] += 1;
                    
                    // Add the block to the list of blocks to write.
                    blocks_to_write.push_back(bgdTable[dir_inode_block_group_num].bg_block_bitmap + i);
                }
            }
        }
        else
        {
            // Need to read all bitmaps.
            for (u32 i = 0; i < numBlockGroups; i++)
            {
                block_group_block_bitmaps.push_back(read_bitmap(bgdTable[i].bg_block_bitmap, superblock.s_blocks_per_group));
            }
            
            // Iterate through all the block groups.
            for (u32 i = 0; i < numBlockGroups; i++)
            {
                // Iterate through the block group's block bitmap.
                for (u64 j = starting_data_block_offset; j < block_group_block_bitmaps[i].size(); j++)
                {
                    // Check that the block is not in use.
                    if (block_group_block_bitmaps[i][j] == false)
                    {
                        // Mark the block as in use.
                        block_group_block_bitmaps[i][j] = true;
                        
                        // Mark the bitmap dirty.
                        dirty_block_group_block_bitmaps[i] = true;
                        
                        // Increment the number of used blocks in this block group.
                        num_blocks_used_per_block_group[i] += 1;
                        
                        // Add the block to the list of blocks to write.
                        blocks_to_write.push_back(bgdTable[i].bg_block_bitmap + i);
                    }
                }
            }
        }
        /***   End make a list of free blocks we plan on using.   ***/
        
        
        /***   Write file to disk.   ***/
        // DUN DUN DUN - the crux of the function
        //
        // algorithm
        //
        // in each block, write block_size_actual bytes worth of data
        
        // Establish a variable to keep track of how many bytes have been written.
        size_t bytes_written = 0;
        
        // Create a buffer for reading the input file.
        s8 * input_file_buffer = nullptr;
        input_file_buffer = new s8[block_size_actual];
        if (input_file_buffer == nullptr)
        {
            cout << "Error: Could not allocate the file buffer. (ext2::file_write)\n";
            throw;
        }
        
        // Dashing through the loop
        // In an integer vector
        // O'er this we traverse
        // Laughing all the way
        // HA HA HA HA
        for (u32 i = 0; i < blocks_to_write.size() - supporting_blocks_needed; i++)
        {
            // Attempt to read a block's worth of bytes from the input file into the input file
            // buffer.  If there aren't a block's worth of bytes left to read, no error occurs and
            // the file buffer will only be partially full.
            input_file.read(input_file_buffer, block_size_actual);
            
            // If the number of bytes to be written is not a full block's worth, ensure the
            // remainder of the buffer is zeroed out.
            if (file_size - bytes_written < block_size_actual)
            {
                // @TODO Verify that numbers do not suffer from an off-by-1 error.
                memset(&(input_file_buffer[file_size - bytes_written]),
                       0,
                       block_size_actual - (file_size - bytes_written));
            }
            
            // Write the contents of the buffer to the file system.
            vdi->vdiSeek(blockToOffset(blocks_to_write[i]), SEEK_SET);
            vdi->vdiWrite(input_file_buffer, block_size_actual);
            
            // Update the number of bytes written.
            bytes_written += block_size_actual;
        }
        
        // Deallocate the input file buffer.
        delete[] input_file_buffer;
        /***   End write file to disk.   ***/
        
        
        /***   Record indirect blocks.   ***/
        // @TODO This NEEDS to be its own function, it's too complicated.
        
        // Declare and allocate a write buffer.
        s8 * write_buffer = nullptr;
        write_buffer = new s8[block_size_actual];
        if (write_buffer == nullptr)
        {
            cout << "Error: Could not allocate write buffer. (ext2::file_write)\n";
            throw;
        }
        
        // Reset the number of bytes written.
        bytes_written = 0;
        
        // Establish variable to keep track of the block pointers of the various indirect blocks
        // written.
        vector<u32> si_blocks_written;
        vector<u32> di_blocks_written;
        vector<u32> ti_blocks_written;
        
        // Establish a variable to keep track of the blocks_to_write index where we are currently
        // pulling from.
        u32 block_index = EXT2_INODE_NBLOCKS_DIR;
        
        // Establish a variable to keep track of the indirect block index.
        u32 indirect_block_index = blocks_to_write.size() - supporting_blocks_needed; 
        
        // Check if there is a need to write the singly indirect block.
        if (blocks_to_write.size() > EXT2_INODE_NBLOCKS_DIR)
        {
            // Determine the total number of singly indirect blocks that will need to be written.
            u32 num_s_blocks_to_write = ((blocks_to_write.size() - EXT2_INODE_NBLOCKS_DIR) % s_num_block_pointers ?
                                         (blocks_to_write.size() - EXT2_INODE_NBLOCKS_DIR) / s_num_block_pointers + 1 :
                                         (blocks_to_write.size() - EXT2_INODE_NBLOCKS_DIR));
            
            // Write all the singly indirect blocks at once.
            for (u32 i = 0; i < num_s_blocks_to_write; i++)
            {
                // Determine the number of bytes to copy from the blocks_to_write vector.
                //
                // This calculation is to determine the number of blocks that will be written, that
                // is, a full block's worth of block pointers, or just until the end of the
                // blocks_to_write vector.
                size_t bytes_to_copy = (EXT2_INODE_NBLOCKS_DIR + i + s_num_block_pointers < blocks_to_write.size() ?
                                        s_num_block_pointers :
                                        blocks_to_write.size() - EXT2_INODE_NBLOCKS_DIR - i);
                // Convert the number of blocks to the number of bytes.
                bytes_to_copy *= EXT2_BLOCK_POINTER_SIZE;
                
                // Copy bytes_to_copy bytes to the write buffer.
                memcpy(write_buffer,
                       &(blocks_to_write.data()[EXT2_INODE_NBLOCKS_DIR + i]),
                       bytes_to_copy);
                
                // Fill the rest of the write buffer with zeroes.
                if (bytes_to_copy < block_size_actual)
                {
                    memset(&(write_buffer[bytes_to_copy]), 0, block_size_actual - bytes_to_copy);
                }
                
                // Seek to and write the block.
                vdi->vdiSeek(blockToOffset(blocks_to_write[indirect_block_index]), SEEK_SET);
                vdi->vdiRead(write_buffer, block_size_actual);
                
                // Record the singly indirect block written.
                si_blocks_written.push_back(blocks_to_write[indirect_block_index]);
                
                // Increment the indirect block index.
                indirect_block_index++;
            }
            
            // Check if we need to write doubly indirect blocks.
            if (si_blocks_written.size() > 1)
            {
                // Determine the total number of doubly indirect blocks that will need to be
                // written.
                u32 num_d_blocks_to_write = ((si_blocks_written.size() - 1) % s_num_block_pointers ?
                                             (si_blocks_written.size() - 1) / s_num_block_pointers + 1 :
                                             (si_blocks_written.size() - 1) / s_num_block_pointers);
                
                // Write all the doubly indirect blocks at once.
                for (u32 i = 0; i < num_d_blocks_to_write; i++)
                {
                    // Determine the number of bytes to copy from the si_blocks_written vector.
                    //
                    // This calculation is to determine the number of blocks that will be written,
                    // that is, a full block's worth of block pointers, or just until the end of the
                    // blocks_to_write vector.
                    size_t bytes_to_copy = (EXT2_INODE_NBLOCKS_DIR + i + s_num_block_pointers < si_blocks_written.size() ?
                                            s_num_block_pointers :
                                            si_blocks_written.size() - EXT2_INODE_NBLOCKS_DIR - i);
                    // Convert the number of blocks to the number of bytes.
                    bytes_to_copy *= EXT2_BLOCK_POINTER_SIZE;
                    
                    // Copy bytes_to_copy bytes to the write buffer.
                    memcpy(write_buffer,
                           &(blocks_to_write.data()[EXT2_INODE_NBLOCKS_DIR + i]),
                           bytes_to_copy);
                    
                    // Fill the rest of the write buffer with zeroes.
                    if (bytes_to_copy < block_size_actual)
                    {
                        memset(&(write_buffer[bytes_to_copy]),
                               0,
                               block_size_actual - bytes_to_copy);
                    }
                    
                    // Seek to and write the block.
                    vdi->vdiSeek(blockToOffset(blocks_to_write[indirect_block_index]), SEEK_SET);
                    vdi->vdiRead(write_buffer, block_size_actual);
                    
                    // Record the singly indirect block written.
                    di_blocks_written.push_back(blocks_to_write[indirect_block_index]);
                    
                    // Increment the indirect block index.
                    indirect_block_index++;
                }
                
                // Check if we need to write triply indirect block. (There will only ever be one
                // triply indirect block, but the same form is kept as with singly- and doubly-
                // indirect blocks because reasons.)
                if (di_blocks_written.size() > 1)
                {
                    // Determine the total number of doubly indirect blocks that will need to be
                    // written.
                    u32 num_t_blocks_to_write = ((di_blocks_written.size() - 1) % s_num_block_pointers ?
                                                 (di_blocks_written.size() - 1) / s_num_block_pointers + 1 :
                                                 (di_blocks_written.size() - 1) / s_num_block_pointers);
                    
                    // Write all the triply indirect blocks at once.
                    for (u32 i = 0; i < num_t_blocks_to_write; i++)
                    {
                        // Determine the number of bytes to copy from the sd_blocks_written vector.
                        //
                        // This calculation is to determine the number of blocks that will be written,
                        // that is, a full block's worth of block pointers, or just until the end of the
                        // blocks_to_write vector.
                        size_t bytes_to_copy = (EXT2_INODE_NBLOCKS_DIR + i + s_num_block_pointers < di_blocks_written.size() ?
                                                s_num_block_pointers :
                                                di_blocks_written.size() - EXT2_INODE_NBLOCKS_DIR - i);
                        // Convert the number of blocks to the number of bytes.
                        bytes_to_copy *= EXT2_BLOCK_POINTER_SIZE;
                        
                        // Copy bytes_to_copy bytes to the write buffer.
                        memcpy(write_buffer,
                               &(blocks_to_write.data()[EXT2_INODE_NBLOCKS_DIR + i]),
                               bytes_to_copy);
                        
                        // Fill the rest of the write buffer with zeroes.
                        if (bytes_to_copy < block_size_actual)
                        {
                            memset(&(write_buffer[bytes_to_copy]),
                                   0,
                                   block_size_actual - bytes_to_copy);
                        }
                        
                        // Seek to and write the block.
                        vdi->vdiSeek(blockToOffset(blocks_to_write[indirect_block_index]), SEEK_SET);
                        vdi->vdiRead(write_buffer, block_size_actual);
                        
                        // Record the singly indirect block written.
                        ti_blocks_written.push_back(blocks_to_write[indirect_block_index]);
                        
                        // Increment the indirect block index.
                        indirect_block_index++;
                    }
                }
            }
        }
        /***   End record indirect blocks.   ***/
        
        
        /***   Find free inode.   ***/
        // Establish a variable to record which inode will be used for the file.
        u32 inode_to_use = 0;
        
        // Establish a variable to hold the inode bitmaps for each of the block group descriptors.
        vector<vector<bool>> block_group_inode_bitmaps;
        
        // Establish a vector to keep track of "dirty" block group inode bitmaps that will need to
        // be written to disk.
        vector<bool> dirty_block_group_inode_bitmaps;
        
        // Establish a vector to keep track of how many indoes are being used from each block group.
        vector<u32> num_inodes_used_per_block_group;
        
        // Add enough elements to the vectors to cover the number of block groups.
        for (u32 i = 0; i < numBlockGroups; i++)
        {
            dirty_block_group_inode_bitmaps.push_back(0);
            num_inodes_used_per_block_group.push_back(0);
        }
        
        // Check if there are any free inodes in the block group where the file's containing
        // directory has its inode.
        if (bgdTable[dir_inode_block_group_num].bg_free_inodes_count > 0)
        {
            // Only need to read one inode bitmap into memory.
            block_group_inode_bitmaps.push_back(read_bitmap(bgdTable[dir_inode_block_group_num].bg_inode_bitmap, superblock.s_inodes_per_group));
            
            // Run through the inode bitmap until a free inode is found.  (With a safety guard
            // against overrunning the bounds of the vector, just in case.)
            for (u32 i = 0; inode_to_use == 0 && i < superblock.s_inodes_per_group; i++)
            {
                if (block_group_inode_bitmaps.back()[i] == false)
                {
                    // Mark that inode as used.
                    block_group_inode_bitmaps.back()[i] = true;
                    
                    // Mark the bitmap dirty.
                    dirty_block_group_inode_bitmaps[dir_inode_block_group_num] = true;
                    
                    // Increment the number of used blocks in this block group.
                    num_inodes_used_per_block_group[dir_inode_block_group_num] += 1;
                    
                    // Calculate the actual inode number.
                    inode_to_use = i + 1 + dir_inode_block_group_num * superblock.s_inodes_per_group;
                }
            }
        }
        else
        {
            // Read all the inode bitmaps into memory.
            for (u32 i = 0; i < numBlockGroups; i++)
            {
                block_group_inode_bitmaps.push_back(read_bitmap(bgdTable[i].bg_inode_bitmap, superblock.s_inodes_per_group));
            }
            
            // Run through the block groups until a free inode is found.  (With a safety guard
            // against overrunning the bounds of the vector, just in case.)
            for (u32 i = 0; inode_to_use == 0 && i < numBlockGroups; i++)
            {
                // Run through the block groups until a free inode is found.  (With a safety guard
                // against overrunning the bounds of the vector, just in case.)
                for (u32 j = 0; inode_to_use == 0 && j < superblock.s_inodes_per_group; j++)
                {
                    if (block_group_inode_bitmaps[i][j] == false)
                    {
                        // Mark that inode as used.
                        block_group_inode_bitmaps[i][j] = true;
                        
                        // Mark the bitmap dirty.
                        dirty_block_group_inode_bitmaps[i] = true;
                        
                        // Increment the number of used blocks in this block group.
                        num_inodes_used_per_block_group[i] += 1;
                        
                        // Calculate the actual inode number.
                        inode_to_use = j + 1 + i * superblock.s_inodes_per_group;
                    }
                }
            }
            
        }
        /***   End find free inode.   ***/
        
        
        /***   Build inode entry.   ***/
        // Create the ext2_inode structure.
        ext2_inode file_inode;
        
        // Zero the entire inode structure since everything is in an indeterminate state initially.
        memset(&file_inode, 0, sizeof(ext2_inode));
        
        // Set the inode type to be "regular file", and then set permissions to 600.
        file_inode.i_mode = EXT2_INODE_TYPE_FILE & EXT2_INODE_PERM_USER_READ & EXT2_INODE_PERM_USER_WRITE;
        
        // Set the user and group to root (user/group 1000)
        file_inode.i_uid = EXT2_INODE_DEFAULT_UID;
        file_inode.i_gid = EXT2_INODE_DEFAULT_GID;
        
        // Set the size.
        file_inode.i_size = file_size;
        
        // Get the current Unix epoch time.
        // NOTE: This is not portable.
        // @TODO Create a utility function to take localtime(time(nullptr)) and make it into Unix
        //       epoch time to enable portability.
        time_t current_time = time(nullptr);
        
        // Set the hard link count.  It will be one since this is just a file.
        file_inode.i_links_count = 1;
        
        // Set the number of 512-byte blocks used to store this file and its data.
        file_inode.i_blocks = total_num_blocks_needed * (block_size_actual / 512);
        
        // The total number of blocks needed may be off if the directory block pointing to the file
        // has insufficient space left to store the ext2_dir_entry structure referencing this file.
        // This calculation compensates for that.
        file_inode.i_blocks -= (additional_dir_block_needed == true ? block_size_actual / 512 : 0);
        
        // Set the access/creation/modification times.
        file_inode.i_atime = current_time;
        file_inode.i_ctime = current_time;
        file_inode.i_mtime = current_time;
        
        // Set the direct block pointers.
        for (u8 i = 0; i < EXT2_INODE_NBLOCKS_DIR && i < blocks_to_write.size(); i++)
        {
            file_inode.i_block[i] = blocks_to_write[i];
        }
        
        // Set the singly indirect block pointer if necessary.
        if (si_blocks_written.size() > 0)
        {
            file_inode.i_block[EXT2_INODE_BLOCK_S_IND] = si_blocks_written.front();
        }
        
        // Set the doubly indirect block pointer if necessary.
        if (di_blocks_written.size() > 0)
        {
            file_inode.i_block[EXT2_INODE_BLOCK_D_IND] = di_blocks_written.front();
        }
        
        //Set the triply indirect block pointer if necessary.
        if (ti_blocks_written.size() > 0)
        {
            file_inode.i_block[EXT2_INODE_BLOCK_T_IND] = ti_blocks_written.front();
        }
        /***   End build inode entry.   ***/
        
        
        /***   Write inode entry to disk.   ***/
        // Seek to the appropriate offset and write the inode to disk.
        vdi->vdiSeek(inodeToOffset(inode_to_use), SEEK_SET);
        vdi->vdiWrite(&file_inode, superblock.s_inode_size);
        /***   End write inode entry to disk.   ***/
        
        
        /***   Build and add directory entry to directory block.   ***/
        // This is going to be a bitch as there are two very different scenarios that could be
        // played out:
        //   1) There's enough space left in the block and the ext2_dir_entry just needs to be
        //      added.  This means that we would have to alter the currently last ext2_dir_entry's
        //      rec_len field accordingly, then add the ext2_dir_entry structure for this file.
        //   2) There's not enough space left in the block and the ext2_dir_entry needs to go into a
        //      new block.  This means the directory inode would need to have its size increased and
        //      the new block would have to be added to the i_block array.  The ext2_dir_entry for
        //      this file will still need to be added to its block, of course.
        
        /***   End build and add directory entry to directory block.   ***/
        
        
        /***   Modify block group descriptor block bitmaps on disk.   ***/
        // Use this loop to check for dirty flags on the block group block bitmaps.
        for (u32 i = 0; i < numBlockGroups; i++)
        {
            // Check for dirty flags indicating that a block bitmap needs to be written to disk.
            if (dirty_block_group_block_bitmaps[i] == true)
            {
                // Write the bitmap to disk.
                write_bitmap(dirty_block_group_block_bitmaps[i], bgdTable[i].bg_block_bitmap);
            }
        }
        /***   End modify block group descriptor block bitmaps on disk.   ***/
        
        
        /***   Modify block group descriptor inode bitmap on disk.   ***/
        // Use this loop to check for dirty flags on the block group inode bitmaps.
        for (u32 i = 0; i < numBlockGroups; i++)
        {
            // Check for dirty flags indicating that an inode bitmap needs to be written to disk.
            if (dirty_block_group_inode_bitmaps[i] == true)
            {
                // Write the bitmap to disk.
                write_bitmap(dirty_block_group_inode_bitmaps[i], bgdTable[i].bg_inode_bitmap);
            }
        }
        /***   End modify block group descriptor inode bitmap on disk.   ***/
        
        
        /***   Modify block group descriptor table in memory.   ***/
        // All this requires is an update to the number of free blocks and inodes.
        for (u32 i = 0; i < numBlockGroups; i++)
        {
            bgdTable[i].bg_free_blocks_count -= num_blocks_used_per_block_group[i];
            bgdTable[i].bg_free_inodes_count -= num_inodes_used_per_block_group[i];
        }
        /***   End modify block group descriptor table in memory.   ***/
        
        
        /***   Modify block group descriptor table on disk.   ***/
        // Run through the block group descriptor table.
        for (u32 i = 0; i < numBlockGroups; i++)
        {
            // Check each block group descriptor to see if they had any dirty bitmaps.
            if (dirty_block_group_block_bitmaps[i] == true || dirty_block_group_inode_bitmaps == true)
            {
                // If they did, the number of blocks and/or the number of inodes that are free have
                // has changed, so write the changed ext2_block_group_desc structure to disk.
                vdi->vdiSeek(bgd_table_start + i * sizeof(ext2_block_group_desc), SEEK_SET);
                vdi->vdiWrite(&(bgdTable[i]), sizeof(ext2_block_group_desc);
            }
        }
        /***   End modify block group descriptor table on disk.   ***/
        
        
        /***   Modify superblock in memory.   ***/
        // Reset the superblock free blocks and inodes count.
        superblock.s_free_blocks_count = 0;
        superblock.s_free_inodes_count = 0;
        
        // Run through the block group descriptor table and total up the number of free blocks and
        // inodes.
        for (u32 i = 0; i < numBlockGroups; i++)
        {
            superblock.s_free_blocks_count += bgdTable[i].bg_free_blocks_count;
            superblock.s_free_inodes_count += bgdTable[i].bg_free_inodes_count;
        }
        
        // Modify the last write time (in Unix epoch time).
        // NOTE: Not portable.
        // @TODO Create utility function to fix portability.
        superblock.s_wtime = time(nullptr);
        /***   End modify superblock in memory.   ***/
        
        
        /***   Modify superblock on disk.   ***/
        // Write the superblock to disk.
        vdi->vdiSeek(superblock_start, SEEK_SET);
        vdi->vdiWrite(&superblock, sizeof(ext2_superblock));
        /***   End modify superblock on disk.   ***/
        
        
        return false; // temp to appease the compiler gods
    }


    /*----------------------------------------------------------------------------------------------
     * Name:    offsetToBlock
     * Type:    Function
     * Purpose: ...
     * Input:   off_t offset, containing ...
     * Output:  u32....
    ----------------------------------------------------------------------------------------------*/
    // Needed?
    u32 ext2::offsetToBlock(off_t offset)
    {
        return (offset - bootSector.partitionTable[0].firstSector * VDI_SECTOR_SIZE) / 
               (EXT2_BLOCK_BASE_SIZE << superblock.s_log_block_size);
    }
    
    
    /*----------------------------------------------------------------------------------------------
     * Name:    inodeToBlockGroup
     * Type:    Function
     * Purpose: Determine which block group an inode is a part of.
     * Input:   u32 inode_number, containing the inode number to check.
     * Output:  u32, containing the block group number the inode is a part of.
    ----------------------------------------------------------------------------------------------*/
    u32 ext2::inodeToBlockGroup(u32 inode_number)
    {
        return (inode_number - 1) / superblock.s_inodes_per_group;
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    inodeBlockGroupIndex
     * Type:    Function
     * Purpose: Determines the index of an inode within a block group.
     * Input:   u32 inode_number, containing the inode number to check.
     * Output:  u32, containing the index number of an inode within a block group.
    ----------------------------------------------------------------------------------------------*/
    u32 ext2::inodeBlockGroupIndex(u32 inode_number)
    {
        return (inode_number - 1) % superblock.s_inodes_per_group;
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    blockToOffset
     * Type:    Function
     * Purpose: ...
     * Input:   u32 block_number, containing ...
     * Output:  off_t....
    ----------------------------------------------------------------------------------------------*/
    off_t ext2::blockToOffset(u32 block_number)
    {
        return block_number < superblock.s_blocks_count ?
               bootSector.partitionTable[0].firstSector * VDI_SECTOR_SIZE + 
                  block_number * (EXT2_BLOCK_BASE_SIZE << superblock.s_log_block_size) :
               -1;
    }
    
    
    /*----------------------------------------------------------------------------------------------
     * Name:    inodeToOffset
     * Type:    Function
     * Purpose: Calculates the offset of an inode on the disk.
     * Input:   u32 inode_num, containing the inode number to calculate the offset for.
     * Output:  off_t, holding the offset of the inode on the disk.
    ----------------------------------------------------------------------------------------------*/
    off_t ext2::inodeToOffset(u32 inode_num)
    {
        u32 inode_block_group = inodeToBlockGroup(inode_num);
        u32 inode_index = inodeBlockGroupIndex(inode_num);
        
        // Calculate and return.
        return blockToOffset(bgdTable[inode_block_group].bg_inode_table) +
               inode_index * superblock.s_inode_size;
    }
    
    
    /*----------------------------------------------------------------------------------------------
     * Name:    print_inode
     * Type:    Function
     * Purpose: Prints inode information.
     * Input:   ext2_inode* inode, containing pointer to an inode object.
     * Output:  Nothing.
    ----------------------------------------------------------------------------------------------*/
    void ext2::print_inode(ext2_inode* inode)
    {
        cout << "\nInode contents:\n";
        cout << "Type: " <<
            (((inode->i_mode >> 12) << 12) == EXT2_INODE_TYPE_FIFO ? "FIFO\n" : "") <<
            (((inode->i_mode >> 12) << 12) == EXT2_INODE_TYPE_CHARDEV ? "Character device\n" : "") <<
            (((inode->i_mode >> 12) << 12) == EXT2_INODE_TYPE_DIR ? "Directory\n" : "") <<
            (((inode->i_mode >> 12) << 12) == EXT2_INODE_TYPE_BLOCKDEV ? "Block device \n" : "") <<
            (((inode->i_mode >> 12) << 12) == EXT2_INODE_TYPE_FILE ? "Regular file\n" : "") <<
            (((inode->i_mode >> 12) << 12) == EXT2_INODE_TYPE_SYMLINK ? "Symbolic link\n" : "") <<
            (((inode->i_mode >> 12) << 12) == EXT2_INODE_TYPE_SOCKET ? "Unix socket\n" : "");
        cout << "Permissions:\n" << 
            (inode->i_mode & EXT2_INODE_PERM_OTHER_EXECUTE ? "  other execute\n" : "") <<
            (inode->i_mode & EXT2_INODE_PERM_OTHER_WRITE ? "  other write\n" : "") <<
            (inode->i_mode & EXT2_INODE_PERM_OTHER_READ ? "  other read\n" : "") <<
            (inode->i_mode & EXT2_INODE_PERM_GROUP_EXECUTE ? "  group execute\n" : "") <<
            (inode->i_mode & EXT2_INODE_PERM_GROUP_WRITE ? "  group write\n" : "") <<
            (inode->i_mode & EXT2_INODE_PERM_GROUP_READ ? "  group read\n" : "") <<
            (inode->i_mode & EXT2_INODE_PERM_USER_EXECUTE ? "  user execute\n" : "") <<
            (inode->i_mode & EXT2_INODE_PERM_USER_WRITE ? "  user write\n" : "") <<
            (inode->i_mode & EXT2_INODE_PERM_USER_READ ? "  user read\n" : "") <<
            (inode->i_mode & EXT2_INODE_PERM_STICKYBIT ? "  sticky bit\n" : "") <<
            (inode->i_mode & EXT2_INODE_PERM_SETGID ? "  set group id\n" : "") <<
            (inode->i_mode & EXT2_INODE_PERM_SETUID ? "  set user id\n" : "");
        cout << "User ID: " << inode->i_uid << endl;
        cout << "Lower 32 bits of size in bytes: " << inode->i_size << endl;
        cout << "Last access time: " << inode->i_atime << endl;
        cout << "Creation time: " << inode->i_ctime << endl;
        cout << "Last modification time: " << inode->i_mtime << endl;
        cout << "Deletion time: " << inode->i_dtime << endl;
        cout << "Group ID: " << inode->i_gid << endl;
        cout << "Hard link count: " << inode->i_links_count << endl;
        cout << "Disk sectors used count: " << inode->i_blocks << endl;
        cout << "Flags:\n" <<
            (inode->i_flags & EXT2_INODE_FLAGS_SECURE_DELETE ? "  secure delete\n" : "") <<
            (inode->i_flags & EXT2_INODE_FLAGS_KEEP_COPY_ON_DELETE ? "  keep copy on delete\n" : "") <<
            (inode->i_flags & EXT2_INODE_FLAGS_FILE_COMPRESSION ? "  file compression\n" : "") <<
            (inode->i_flags & EXT2_INODE_FLAGS_SYNC_UPDATES ? "  synchronous updates\n" : "") <<
            (inode->i_flags & EXT2_INODE_FLAGS_IMMUTABLE ? "  immutable file\n" : "") <<
            (inode->i_flags & EXT2_INODE_FLAGS_APPEND ? "  append only\n" : "") <<
            (inode->i_flags & EXT2_INODE_FLAGS_DO_NOT_DUMP ? "  file not included in 'dump' commend\n" : "") <<
            (inode->i_flags & EXT2_INODE_FLAGS_LAST_ACCESS_NO_UPDATE ? "  last access time not updated\n" : "") <<
            (inode->i_flags & EXT2_INODE_FLAGS_HASH_INDEX_DIR ? "  hash indexed directory\n" : "") <<
            (inode->i_flags & EXT2_INODE_FLAGS_AFS_DIR ? "  AFS directory\n" : "") <<
            (inode->i_flags & EXT2_INODE_FLAGS_JOURNAL_FILE_DATA ? "  journal file data\n" : "");
        for (u32 i = 0; i < EXT2_INODE_NBLOCKS_TOT; i++)
            cout << "Block pointer " << i << ": " << inode->i_block[i] << endl;
        cout << "File version: " << inode->i_generation << endl;
        cout << "File ACL: " << inode->i_file_acl << endl;
        cout << "Directory ACL: " << inode->i_dir_acl << endl;
        cout << "Fragment address: " << inode->i_faddr << endl;
    }
    
    
    /*----------------------------------------------------------------------------------------------
     * Name:    print_dir_entry
     * Type:    Function
     * Purpose: Prints directory information.
     * Input:   ext2_dir_entry & dir_entry, containing pointer to a directory object.
     * Output:  Nothing.
    ----------------------------------------------------------------------------------------------*/
    void ext2::print_dir_entry(ext2_dir_entry & dir_entry, bool verbose)
    {
        if (verbose)
        {
            cout << "\nDirectory Entry:\n";
            cout << "Inode number: " << dir_entry.inode << endl;
            cout << "Directory record length: " << dir_entry.rec_len << endl;
            cout << "Name length: " << (int)(dir_entry.name_len) << endl;
            cout << "Directory entry type: " <<
                (dir_entry.file_type == EXT2_DIR_TYPE_UNKNOWN ? "unknown\n" : "") <<
                (dir_entry.file_type == EXT2_DIR_TYPE_FILE ? "regular file\n" : "") <<
                (dir_entry.file_type == EXT2_DIR_TYPE_DIR ? "directory\n" : "") <<
                (dir_entry.file_type == EXT2_DIR_TYPE_CHARDEV ? "character device\n" : "") <<
                (dir_entry.file_type == EXT2_DIR_TYPE_BLOCKDEV ? "block device\n" : "") <<
                (dir_entry.file_type == EXT2_DIR_TYPE_FIFO ? "FIFO\n" : "") <<
                (dir_entry.file_type == EXT2_DIR_TYPE_SOCKET ? "socket\n" : "") <<
                (dir_entry.file_type == EXT2_DIR_TYPE_SYMLINK ? "symbolic link\n" : "");
            cout << "Name: ";
        }
        
        cout << dir_entry.name << endl;
    }
    
    
    /*----------------------------------------------------------------------------------------------
     * Name:    print_superblock
     * Type:    Function
     * Purpose: Prints superblock information.
     * Input:   Nothing.
     * Output:  Nothing.
    ----------------------------------------------------------------------------------------------*/
    void ext2::print_superblock()
    {
        cout << "\nSuperblock Dump:\n";
        cout << "Inodes count: " << superblock.s_inodes_count << endl;
        cout << "Blocks count: " << superblock.s_blocks_count << endl;
        cout << "Reserved blocks count: " << superblock.s_r_blocks_count << endl;
        cout << "Free blocks count: " << superblock.s_free_blocks_count << endl;
        cout << "Free inodes count: " << superblock.s_free_inodes_count << endl;
        cout << "First data block: " << superblock.s_first_data_block << endl;
        cout << "Block size: " << (EXT2_BLOCK_BASE_SIZE << superblock.s_log_block_size) << endl;
        cout << "Fragment size: " << (EXT2_FRAG_BASE_SIZE << superblock.s_log_frag_size) << endl;
        cout << "# blocks per group: " << superblock.s_blocks_per_group << endl;
        cout << "# fragments per group: " << superblock.s_frags_per_group << endl;
        cout << "# inodes per group: " << superblock.s_inodes_per_group << endl;
        cout << "Mount time: " << superblock.s_mtime << endl;
        cout << "Write time: " << superblock.s_wtime << endl;
        cout << "Mount count: " << superblock.s_mnt_count << endl;
        cout << "Maximal mount count: " << superblock.s_max_mnt_count << endl;
        cout << "Magic signature: " << hex << superblock.s_magic << dec << endl;
        cout << "File system state: " << superblock.s_state << endl;
        cout << "Error behaviour: " << superblock.s_errors << endl;
        cout << "Minor revision level: " << superblock.s_minor_rev_level << endl;
        cout << "Time of last check: " << superblock.s_lastcheck << endl;
        cout << "Check interval: " << superblock.s_checkinterval << endl;
        cout << "Creator OS: " << superblock.s_creator_os << endl;
        cout << "Revision level: " << superblock.s_rev_level << endl;
        cout << "Default reserved UID: " << superblock.s_def_resuid << endl;
        cout << "Default reserved GID: " << superblock.s_def_resgid << endl;
        
        cout << "\nFirst non-reserved inode: " << superblock.s_first_ino << endl;
        cout << "inode structure size: " << superblock.s_inode_size << endl;
        cout << "Block group number of superblock: " << superblock.s_block_group_nr << endl;
        cout << "Compatible feature set: " << superblock.s_feature_compat << endl;
        cout << "Incompatible feature set: " << superblock.s_feature_incompat << endl;
        cout << "Read-only compatible feature set: " << superblock.s_feature_ro_compat << endl;
        cout << "Volume UUID: " << superblock.s_uuid << endl;
        cout << "Volume Name: " << superblock.s_volume_name << endl;
        cout << "Directory where last mounted: " << superblock.s_last_mounted << endl;
        cout << "Compression algorithm usage bitmap: " << superblock.s_algorithm_usage_bitmap << endl;
    }
    
    
    /*----------------------------------------------------------------------------------------------
     * Name:    print_bootsector
     * Type:    Function
     * Purpose: Prints bootsector information.
     * Input:   Nothing.
     * Output:  Nothing.
    ----------------------------------------------------------------------------------------------*/
    void ext2::print_bootsector()
    {
        cout << "\nBootSector struct: ";
        for (int i = 0; i < 4 /*number of partition tables*/; i++) {
            cout << "\nPartition Entry " << i+1 << ": ";
            cout << "\n  First Sector: " << bootSector.partitionTable[i].firstSector;
            cout << "\n  nSectors: " << bootSector.partitionTable[i].nSectors;
        }
        cout << "\nBoot Sector Signature: " << hex << bootSector.magic << dec << endl;
    }
    
    
    /*----------------------------------------------------------------------------------------------
     * Name:    print_bgd_table
     * Type:    Function
     * Purpose: Prints the BGD table information.
     * Input:   Nothing.
     * Output:  Nothing.
    ----------------------------------------------------------------------------------------------*/
    void ext2::print_bgd_table()
    {
        cout << "\nBGD Table:\n";
        for (u32 i = 0; i < numBlockGroups; i++)
        {
            cout << "Block Group " << i << endl;
            cout << "  Block address of block usage bitmap: " << bgdTable[i].bg_block_bitmap << endl;
            cout << "  Block address of inode usage bitmap: " << bgdTable[i].bg_inode_bitmap << endl;
            cout << "  Starting block address of inode table: " << bgdTable[i].bg_inode_table << endl;
            cout << "  Number of unallocated blocks in group: " << bgdTable[i].bg_free_blocks_count << endl;
            cout << "  Number of unallocated inodes in group: " << bgdTable[i].bg_free_inodes_count << endl;
            cout << "  Number of directories in group: " << bgdTable[i].bg_used_dirs_count << endl;
        }
    }
    
    
    /*----------------------------------------------------------------------------------------------
     * Name:    parse_directory_inode
     * Type:    Function
     * Purpose: Read and parse a directory inode, returning the resulting list of directories and
     *          files in a vector container.
     * Input:   ext2_inode inode, containing an inode object.
     * Output:  vector<ext2_dir_entry>, contains a vector holding the contents of the inode.
    ----------------------------------------------------------------------------------------------*/
    vector<ext2::ext2_dir_entry> ext2::parse_directory_inode(ext2_inode inode)
    {
        vector<ext2_dir_entry> to_return;
        u32 cursor = 0;
        s8 name_buffer[256];
        u8* inode_buffer = nullptr;
        
        // Attempt to allocate and then verify memory for the inode buffer.
        inode_buffer = new u8[EXT2_BLOCK_BASE_SIZE << superblock.s_log_block_size];
        if (inode_buffer == nullptr)
        {
            cout << "Error allocating directory inode buffer.";
            throw;
        }
        
        // Iterate through the direct block pointer portion of the directory inode's i_block array.
        for (s32 i = 0; i < EXT2_INODE_NBLOCKS_DIR; i++)
        {
            // Checks to make sure the i_block entry actually points somewhere.
            if (inode.i_block[i] == 0)
            {
                // If it does not, continue to the next block pointer.
                continue;
            }
            
            // Set the offset to the beginning of the block referenced by the inode.
            vdi->vdiSeek(blockToOffset(inode.i_block[i]), SEEK_SET);
            
            // Read the contents of the block into memory, based on the given size.
            vdi->vdiRead(inode_buffer, EXT2_BLOCK_BASE_SIZE << superblock.s_log_block_size);
            
            // Iterate through the inode buffer, reading the directory entry records.
            while (cursor < (EXT2_BLOCK_BASE_SIZE << superblock.s_log_block_size))
            {
                // Declare a new ext2_dir_entry every loop.
                ext2_dir_entry to_add;
                
                // Read inode, record length, name length, and file type.
                memcpy(&to_add, &(inode_buffer[cursor]), EXT2_DIR_BASE_SIZE);
                
                // Check if the name length is 0.  If so, skip record.
                if (to_add.name_len == 0)
                {
                    cursor += to_add.rec_len;
                    continue;
                }
                else
                {
                    cursor += EXT2_DIR_BASE_SIZE;
                }

                // Read the name into the buffer.
                memcpy(name_buffer, &(inode_buffer[cursor]), to_add.name_len);
                cursor += to_add.name_len;
                
                // Add a null to the end of the characters read in so it becomes a C-style string.
                name_buffer[to_add.name_len] = '\0';
                
                // Store the name in the record.
                to_add.name.assign(name_buffer);
                
                // Set the offset to the next record.
                if (EXT2_DIR_BASE_SIZE + to_add.name_len < to_add.rec_len)
                    cursor += to_add.rec_len - EXT2_DIR_BASE_SIZE - to_add.name_len;
                
                // Add the ext2_dir_entry to the vector.
                to_return.push_back(to_add);
                
                cout << "\ndebug::ext2::ext2_dir_entry\n";
                print_dir_entry(to_return.back());
            }
        }
        
        // Free the inode buffer.
        delete[] inode_buffer;
        
        // Return the vector.
        return to_return;
    }
    
    
    /*----------------------------------------------------------------------------------------------
     * Name:    parse_directory_inode
     * Type:    Function
     * Purpose: Read and parse a directory inode, returning the resulting list of directories and
     *          files in a vector container.
     * Input:   u32 inode_num, containing the inode number of a directory inode.
     * Output:  vector<ext2_dir_entry>, contains a vector holding the contents of the directory
     *          inode.
    ----------------------------------------------------------------------------------------------*/
    vector<ext2::ext2_dir_entry> ext2::parse_directory_inode(u32 inode_num)
    {
        return parse_directory_inode(readInode(inode_num));
    }
    
    
    /*----------------------------------------------------------------------------------------------
     * Name:    readInode
     * Type:    Function
     * Purpose: Small function that verifies an inode index is in bounds, then reads it.
     * Input:   u32 inode, containing an inode number.
     * Output:  ext2_inode, containing the read inode.
     *
     * @TODO    Set up an actual error to throw instead of the generic one.
    ----------------------------------------------------------------------------------------------*/
    ext2::ext2_inode ext2::readInode(u32 inode)
    {
        if (inode >= superblock.s_inodes_count)
        {
            cout << "inode out of bounds\n";
            throw;
        }
        
        ext2_inode to_return;
        
        vdi->vdiSeek(inodeToOffset(inode), SEEK_SET);
        vdi->vdiRead(&to_return, sizeof(ext2_inode));
        
        return to_return;
    }
    

    /*----------------------------------------------------------------------------------------------
     * Name:    dir_entry_exists
     * Type:    Function
     * Purpose: Verifies whether a path exists or not.
     * Input:   const string & path_to_check, containing the path to verify.
     * Output:  vector<ext2_dir_entry>, containing ext2_dir_entry objects representing the path to
     *          the desired folder if it is found, or containing nothing if not found.
    ----------------------------------------------------------------------------------------------*/
    vector<ext2::ext2_dir_entry> ext2::dir_entry_exists(const string & path_to_check)
    {
        // Tokenize the path that will be checked.
        vector<string> path_tokens = utility::tokenize(path_to_check, DELIMITER_FSLASH);
        
        vector<ext2_dir_entry> to_return;
        
        // Check if the given path is relative or absolute.
        if(path_to_check[0] == '/')
        {
            // Absolute path given.
            to_return.push_back(pwd.front());
        }
        else
        {
            // Relative path given.
            to_return = pwd;
        }
        
        vector<ext2_dir_entry> current_dir_parse;
        bool found_dir = true;
        
        // Loop through the path_tokens vector, following the user's desired path sequentially.
        for (u32 i = 0; found_dir == true && i < path_tokens.size(); i++)
        {
            // Parse the current inode for its directory structure.
            current_dir_parse = parse_directory_inode(to_return.back().inode);
            
            // Reset the found directory flag on each iteration.
            found_dir = false;
            
            // Loop through the parsed directory listing to attempt to find a match.
            for (u32 j = 0; j < current_dir_parse.size(); j++)
            {
                // First, check if the directory entry is a folder.
                if (current_dir_parse[j].file_type == 2)
                {
                    // Then check if the names match.
                    if (path_tokens[i] == current_dir_parse[j].name)
                    {
                        // If the directory entry is a folder and the names match, handle the
                        // different scenarios.
                        if (path_tokens[i] == ".")
                        {
                            // Intentionally do nothing.
                        }
                        else if (path_tokens[i] == "..")
                        {
                            // Go up one level unless already at the filesystem root.
                            if (to_return.size() > 1)
                            {
                                to_return.pop_back();
                            }
                        }
                        else
                        {
                            // Add the directory entry to the to_return vector.
                            to_return.push_back(current_dir_parse[j]);
                        }
                        
                        // Set the found directory flag.
                        found_dir = true;
                        break;
                    }
                }
            }
        }
        
        // If the directory was not found, clear the to_return vector.
        if (found_dir == false)
        {
            to_return.clear();
        }
        
        // Return the path to the folder.
        return to_return;
    } 
    
    
    /*----------------------------------------------------------------------------------------------
     * Name:    file_entry_exists
     * Type:    Function
     * Purpose: Verifies whether a file exists or not. Only checks in the pwd.
     * Input:   const string & file_to_check, containing the path to verify.
     * Output:  <reference> u32 & file_inode, will hold the file's inode number, should the file
     *          exist.
     * Output:  bool, detailing whether the file exists (true) or does not (false).
    ----------------------------------------------------------------------------------------------*/
    bool ext2::file_entry_exists(const string & file_to_check, u32 & file_inode)
    {
        // Assume the file does not exist.
        bool to_return = false;
        
        // Parse the present working directory.
        vector<ext2_dir_entry> file_path = parse_directory_inode(pwd.back().inode);
        
        // Loop through the contents of the pwd looking for a name match.
        for (u32 i = 0; i < file_path.size(); i++)
        {
            // Check the name, and if the name matches, make sure it's not a directory.
            if (file_path[i].name == file_to_check && file_path[i].file_type != EXT2_DIR_TYPE_DIR)
            {
                // Assuming the entry is indeed a file, store the file's inode and update the return
                // value.
                file_inode = file_path[i].inode;
                to_return = true;
            }
        }
        
        // Return whether the file exists or not.
        return to_return;
        
        // Below is code to enable pathing support in the function.  It is currently not working and
        // commented out due to needing to work on other more important things.
        
        /***   START ACTUAL CODE   ***/
        // @TODO convert code from dir_entry_exists
        // @TODO do a deep examination of the algorithm to ensure it's appropriate for files
        
        // tokenize
        // determine if relative path or local path
        // loop through the path vector
        
        
        // Tokenize the path that will be checked.
        // vector<string> path_tokens = utility::tokenize(file_to_check, DELIMITER_FSLASH);
        
        // vector<ext2_dir_entry> file_path;
        
        // // Check if the given path is relative or absolute.
        // if (file_to_check[0] == '/')
        // {
        //     // Absolute path given.
        //     file_path.push_back(pwd.front());
        // }
        // else
        // {
        //     // Relative path given.
        //     file_path = pwd;
        // }
        
        // vector<ext2_dir_entry> current_dir_parse;
        // bool found_dir_entry = true;
        
        // // Loop through the path_tokens vector, following the user's desired path sequentially.
        // for (u32 i = 0; found_dir_entry == true && i < path_tokens.size(); i++)
        // {
        //     // Parse the current inode for its directory structure.
        //     current_dir_parse = parse_directory_inode(file_path.back().inode);
            
        //     // Reset the found directory flag on each iteration.
        //     found_dir_entry = false;
            
        //     // Loop through the parsed directory listing to attempt to find a match.
        //     for (u32 j = 0; j < current_dir_parse.size(); j++)
        //     {
        //         // Check if the names match.
        //         if (path_tokens[i] == current_dir_parse[j].name)
        //         {
        //             // Check if the directory entry is a folder.
        //             if (current_dir_parse[j].file_type == 2)
        //             {
        //                 // If the directory entry is a folder handle the different scenarios.
        //                 if (path_tokens[i] == ".")
        //                 {
        //                     // Intentionally do nothing.
        //                 }
        //                 else if (path_tokens[i] == "..")
        //                 {
        //                     // Go up one level unless already at the filesystem root.
        //                     if (file_path.size() > 1)
        //                     {
        //                         file_path.pop_back();
        //                     }
        //                 }
        //                 else
        //                 {
        //                     // Add the directory entry to the file_path vector.
        //                     file_path.push_back(current_dir_parse[j]);
        //                 }
                        
        //                 // Set the found directory flag.
        //                 found_dir_entry = true;
        //                 break;
        //             }
        //             else if (current_dir_parse[j].file_type == 2)
        //             {
                        
        //             }
        //         }
        //     }
        // }
        
        // // If the directory was not found, clear the file_path vector.
        // if (found_dir_entry == false)
        // {
        //     file_inode = file_path.back().inode;
        // }
        
        // // Return true if the file was found, false if not.
        // return found_dir_entry;
        /***   END ACTUAL CODE   ***/
    }
    
    
    void ext2::debug_dump_pwd_inode()
    {
        ext2_inode temp = readInode(pwd.back().inode);
        print_inode(&temp);
    }
    
    
    void ext2::print_block(u32 block_to_dump, bool text)
    {
        u8 * raw_block = new u8[EXT2_BLOCK_BASE_SIZE << superblock.s_log_block_size];
        
        vdi->vdiSeek(blockToOffset(block_to_dump), SEEK_SET);
        vdi->vdiRead(raw_block, (EXT2_BLOCK_BASE_SIZE << superblock.s_log_block_size));
        
        cout << "ext2::print_block\n";
        
        if (!text)
        {
            for (u32 i = 0; i < (EXT2_BLOCK_BASE_SIZE << superblock.s_log_block_size); i++)
            {
                cout << hex << setw(2) << setfill('0') << (int)raw_block[i] << ' ';
            }
        }
        else
        {
            for (u32 i = 0; i < (EXT2_BLOCK_BASE_SIZE << superblock.s_log_block_size); i++)
            {
                cout << raw_block[i] << ' ';
            }
        }
        cout << dec;
        
        delete[] raw_block;
    }
    
    
    void ext2::debug_dump_block(u32 block_to_dump)
    {
        print_block(block_to_dump);
    }
    
    
    void ext2::debug_dump_inode(u32 inode_to_dump)
    {
        ext2_inode inode = readInode(inode_to_dump);
        print_inode(&inode);
    }
    
    
    /*----------------------------------------------------------------------------------------------
     * Name:    make_block_list
     * Type:    Function
     * Purpose: Unrolls an inode, reading all the different blocks associated with a file, including
     *          direct, singly indirect, doubly indirect, and triply indirect.  Will return a list
     *          containing all of these block numbers, in order, so the file can be stitched
     *          together.
     * Input:   const u32 inode_number, holding the number of the initial inode.
     * Output:  list<u32>, containing a list of all the different block numbers where the file is
     *          contained.
    ----------------------------------------------------------------------------------------------*/
    list<u32> ext2::make_block_list(const u32 inode_number)
    {
        list<u32> to_return;
        
        u32 * s_ind_block_buffer = nullptr;
        u32 * d_ind_block_buffer = nullptr;
        u32 * t_ind_block_buffer = nullptr;
        
        // general algorithm for this function
        // NOTE: THIS IS BRUTE FORCE AND UGLY AND NOT AT ALL OPTIMIZED AND MAKES ME FEEL DIRTY
        // @TODO: Optimize.
        // 
        // read inode
        // add direct block pointers to list
        // read singly indirect block pointer
        //   add direct block pointers to list
        // read doubly indirect block pointer
        //   read singly indirect block pointers
        //     add direct block pointers to list
        // read triply indirect block pointer
        //   read doubly indirect block pointers
        //     read singly indirect block pointers
        //       add direct block pointers to list
        
        // Read the inode.
        ext2_inode inode = readInode(inode_number);
        
        // Direct
        // Add the direct blocks contained in the inode entry itself.
        for (u32 i = 0; i < EXT2_INODE_NBLOCKS_DIR || inode.i_block[i] == 0; i++)
        {
            // Add the block to the block list.
            to_return.push_back(inode.i_block[i]);
        }
        
        // Singly Indirect
        // Get and parse the singly indirect block and add its contents to the block list.
        if (inode.i_block[EXT2_INODE_BLOCK_S_IND] != 0)
        {
            // Allocate (and check) some memory for the singly indirect block buffer.
            s_ind_block_buffer = new u32[block_size_actual / sizeof(u32)];
            if (s_ind_block_buffer == nullptr)
            {
                cout << "Error allocating memory for the singly indirect block buffer.\n";
                throw;
            }
            
            // Set the cursor to and read from the particular block in question.
            vdi->vdiSeek(blockToOffset(inode.i_block[EXT2_INODE_BLOCK_S_IND]), SEEK_SET);
            vdi->vdiRead(s_ind_block_buffer, block_size_actual);
            
            // Iterate through the read block and add the block numbers to the list.  Stop if an
            // entry of 0 is encountered.
            for (u32 i = 0; i < block_size_actual / sizeof(u32) && s_ind_block_buffer[i] != 0; i++)
            {
                // Add the block to the block list.
                to_return.push_back(s_ind_block_buffer[i]);
            }
        }
        
        // Doubly Indirect
        // Get and parse the doubly indirect block to get the list of singly indirect blocks, then
        // parse them and add their contents to the block list.
        if (inode.i_block[EXT2_INODE_BLOCK_D_IND] != 0)
        {
            // Allocate (and check) some memory for the doubly indirect block buffer.
            d_ind_block_buffer = new u32[block_size_actual / sizeof(u32)];
            if (d_ind_block_buffer == nullptr)
            {
                cout << "Error allocating memory for the doubly indirect block buffer.\n";
                throw;
            }
            
            // Set the cursor to and read from the particular block in question.
            vdi->vdiSeek(blockToOffset(inode.i_block[EXT2_INODE_BLOCK_D_IND]), SEEK_SET);
            vdi->vdiRead(d_ind_block_buffer, block_size_actual);
            
            // Iterate through the read block, parsing the singly indirect blocks that are found.
            // Stop if an entry of 0 is encountered.
            for (u32 j = 0; j < block_size_actual / sizeof(u32) && d_ind_block_buffer[j] != 0; j++)
            {
                // Set the cursor to and read from the particular block in question.
                vdi->vdiSeek(blockToOffset(d_ind_block_buffer[j]), SEEK_SET);
                vdi->vdiRead(s_ind_block_buffer, block_size_actual);
                
                // Iterate through the read block and add the block numbers to the list.  Stop if an
                // entry of 0 is encountered.
                for (u32 i = 0; i < block_size_actual / sizeof(u32) && s_ind_block_buffer[i] != 0; i++)
                {
                    // Add the block to the block list.
                    to_return.push_back(s_ind_block_buffer[i]);
                }
            }
        }
        
        // Triply Indirect
        // Get and parse the triply indirect block to get the list of doubly indirect blocks to get
        // the list of singly indirect blocks, then parse them and add their contents to the block
        // list.
        if (inode.i_block[EXT2_INODE_BLOCK_T_IND] != 0)
        {
            // Allocate (and check) some memory for the triply indirect block buffer.
            t_ind_block_buffer = new u32[block_size_actual / sizeof(u32)];
            if (t_ind_block_buffer == nullptr)
            {
                cout << "Error allocating memory for the triply indirect block buffer.\n";
                throw;
            }
            
            // Set the cursor to and read from the particular block in question.
            vdi->vdiSeek(blockToOffset(inode.i_block[EXT2_INODE_BLOCK_T_IND]), SEEK_SET);
            vdi->vdiRead(t_ind_block_buffer, block_size_actual);
            
            // Iterate through the read block, parsing the doubly indirect blocks that are found.
            // Stop if an entry of 0 is encountered.
            for (u32 k = 0; k < block_size_actual / sizeof(u32) && t_ind_block_buffer[k] != 0; k++)
            {
                // Set the cursor to and read from the particular block in question.
                vdi->vdiSeek(blockToOffset(t_ind_block_buffer[k]), SEEK_SET);
                vdi->vdiRead(d_ind_block_buffer, block_size_actual);
                
                // Iterate through the read block, parsing the singly indirect blocks that are
                // found. Stop if an entry of 0 is encountered.
                for (u32 j = 0; j < block_size_actual / sizeof(u32) && d_ind_block_buffer[j] != 0; j++)
                {
                    // Set the cursor to and read from the particular block in question.
                    vdi->vdiSeek(blockToOffset(d_ind_block_buffer[j]), SEEK_SET);
                    vdi->vdiRead(s_ind_block_buffer, block_size_actual);
                    
                    // Iterate through the read block and add the block numbers to the list.  Stop
                    // if an entry of 0 is encountered.
                    for (u32 i = 0; i < block_size_actual / sizeof(u32) && s_ind_block_buffer[i] != 0; i++)
                    {
                        // Add the block to the block list.
                        to_return.push_back(s_ind_block_buffer[i]);
                    }
                }
            }
        }
        
        // Clean up the buffers.
        if (s_ind_block_buffer)
            delete[] s_ind_block_buffer;
        if (d_ind_block_buffer)
            delete[] d_ind_block_buffer;
        if (t_ind_block_buffer)
            delete[] t_ind_block_buffer;
        
        cout << "debug (ext2::make_block_list) leaving function\n";
        
        // Return the block list.
        return to_return;
    }
    
    
    /*----------------------------------------------------------------------------------------------
     * Name:    make_dir_entry
     * Type:    Function
     * Purpose: Takes the inputs given and returns an ext2_dir_entry structure.
     * Input:   const u32 inode_number, holding the number of the inode to be included.
     * Input:   const string & filename, holding the filename to be included.
     * Input:   const u8 filetype, holding the filetype to be included.
     * Output:  ext2_dir_entry, containing the resulting ext2_dir_entry.
    ----------------------------------------------------------------------------------------------*/
    ext2::ext2_dir_entry ext2::make_dir_entry(const u32 inode_number, const string & filename, const u8 filetype)
    {
        ext2_dir_entry to_return;
        
        // Set the majority of the fields in the the to_return variable.
        to_return.inode = inode_number;
        to_return.file_type = filetype;
        to_return.name_len = filename.length();
        to_return.name.assign(filename);
        
        // Calculate the record length and then pad it to make sure it aligns to a 4-byte boundary.
        to_return.rec_len = EXT2_DIR_BASE_SIZE + to_return.name_len;
        to_return.rec_len += to_return.rec_len % 4;
        
        // Return the newly-minted ext2_dir_entry structure.
        return to_return;
    }
    
    
    /*----------------------------------------------------------------------------------------------
     * Name:    read_bitmap
     * Type:    Function
     * Purpose: Reads a bitmap into memory and returns one that is a bit easier to work with, that
     *          is, one that is simply a vector of type bool.
     * Input:   const u32 block_num, holds the block that has the bitmap.
     * Input:   const u64 num_bitmap_entries, holds the number of entries in a bitmap.
     * Output:  vector<bool>, holds the more-easily-referenced bitmap.
     *
     * @TODO    Create another more abstract version of this and put it in the utility namespace.
    ----------------------------------------------------------------------------------------------*/
    vector<bool> ext2::read_bitmap(const u32 block_num, const u64 num_bitmap_entries)
    {
        vector<bool> to_return;
        
        // Convert the number of entries in the bitmap to the size of the bitmap in bytes.
        size_t bitmap_size = (num_bitmap_entries % BITS_PER_BYTE ?
                              num_bitmap_entries / BITS_PER_BYTE + 1 :
                              num_bitmap_entries / BITS_PER_BYTE);
        
        // Create a buffer for reading the bitmap block.
        u8 * bitmap_block_buffer = nullptr;
        bitmap_block_buffer = new u8[bitmap_size];
        if (bitmap_block_buffer == nullptr)
        {
            cout << "Error: Error allocating the bitmap block buffer. (ext2::read_bitmap)\n";
            throw;
        }
        
        // Seek to and read the bitmap into the buffer.
        vdi->vdiSeek(blockToOffset(block_num), SEEK_SET);
        vdi->vdiRead(bitmap_block_buffer, bitmap_size);
        
        // Process the bitmap.
        for (u32 i = 0; i < num_bitmap_entries; i++)
        {
            // Access byte number i/BITS_PER_BYTE, bitshift it i%BITS_PER_BYTE to the left to get
            // rid of any more significant bits, ensuring the bit we want is now in the most
            // significant position itself, then bitshift it 7 to the right to get rid of any less
            // significant bits, making the value either true or false.
            to_return.push_back((bitmap_block_buffer[i / BITS_PER_BYTE] << (i % BITS_PER_BYTE)) >> 7);
        }
        
        // Deallocate the bitmap block buffer.
        delete[] bitmap_block_buffer;
        
        // Return the vectored bitmap.
        return to_return;
    }
    
    
    /*----------------------------------------------------------------------------------------------
     * Name:    write_bitmap
     * Type:    Function
     * Purpose: Writes a bitmap to disk.
     * Input:   const vector<bool> & bitmap_vector, holds the bitmap in vector form.
     * Input:   const u32 block_num, holds the block number to write the bitmap to.
     * Output:  Nothing.
     *
     * @TODO    Create another more abstract version of this and put it in the utility namespace.
    ----------------------------------------------------------------------------------------------*/
    void ext2::write_bitmap(const vector<bool> & bitmap_vector, const u32 block_num)
    {
        // Create a buffer for writing the bitmap block.
        u8 * bitmap_block_buffer = nullptr;
        bitmap_block_buffer = new u8[block_size_actual];
        if (bitmap_block_buffer == nullptr)
        {
            cout << "Error: Error allocating the bitmap block buffer. (ext2::write_bitmap)\n";
            throw;
        }
        
        // Zero out the buffer to ensure a pristine workspace.
        memset(bitmap_block_buffer, 0, block_size_actual);
        
        // Copy the data from the bitmap vector into the buffer.
        for (u32 i = 0; i < bitmap_vector.size(); i++)
        {
            // Access byte number i/BITS_PER_BYTE, bitshift it 1 to the left then bitwise OR it with
            // whatever the bitmap vector value is at index i.
            bitmap_block_buffer[i / BITS_PER_BYTE] = (bitmap_block_buffer[i / BITS_PER_BYTE] << 1) | bitmap_vector[i];
        }
        
        // Seek to and write the bitmap to disk.
        vdi->vdiSeek(blockToOffset(block_num), SEEK_SET);
        vdi->vdiWrite(bitmap_block_buffer, block_size_actual);
        
        // Deallocate the bitmap block buffer.
        delete[] bitmap_block_buffer;
    }
} // namespace vdi_explorer