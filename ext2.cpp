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
#include "vdi_reader.h"
#include <iostream>
#include <iomanip>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include "utility.h"

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
        
        // Read the superblock.
        vdi->vdiSeek(bootSector.partitionTable[0].firstSector * VDI_SECTOR_SIZE +
                     EXT2_SUPERBLOCK_OFFSET, SEEK_SET);
        vdi->vdiRead(&superBlock, sizeof(ext2_superblock));
        
        // Debug info.
        print_superblock();
        // End debug info.
        
        // Calculate and verify the number of block groups in the partition.
        u32 nBlockGroupCalc1, nBlockGroupCalc2;
        nBlockGroupCalc1 = (float)superBlock.s_blocks_count / superBlock.s_blocks_per_group + 0.5;
        nBlockGroupCalc2 = (float)superBlock.s_inodes_count / superBlock.s_inodes_per_group + 0.5;
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
        off_t bgdTableStart = superBlock.s_log_block_size == 0 ?
                              blockToOffset(2) :
                              blockToOffset(1);
        
        // Debug info.
        cout << "BGD Table Start: " << bgdTableStart << endl;
        // End debug info.
        
        // Read the block group descriptor table into memory.
        vdi->vdiSeek(bgdTableStart, SEEK_SET);
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
     * @TODO    Investigate segfault in "/lost+found". *** HIGH ***
    ----------------------------------------------------------------------------------------------*/
    vector<fs_entry_posix> ext2::get_directory_contents(void)
    {
        // stub
        vector<fs_entry_posix> to_return;
        ext2_inode temp_inode;
        
        vector<ext2_dir_entry> directory_contents = parse_directory_inode(pwd.back().inode);

        for (u32 i = 0; i < directory_contents.size(); i++)
        {
            temp_inode = readInode(directory_contents[i].inode);
            
            to_return.emplace_back();
            to_return.back().name = directory_contents[i].name;
            // to_return.back().type = (temp_inode.i_mode & 0xf000) >> 12 ; // mask for the top 4 bits
            to_return.back().type = directory_contents[i].file_type;
            to_return.back().permissions = temp_inode.i_mode & 0x0fff; // mask for the bottom 12 bits
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
     * @TODO    Add proper handling for full paths and relative paths beyond one level away.
     * @TODO    Add proper handling of '.' and '..' directories.
     * #TODO    Investigate segfault after some number of '..' directory changes. *** HIGH ***
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
               (EXT2_BLOCK_BASE_SIZE << superBlock.s_log_block_size);
    }
    
    
    /*----------------------------------------------------------------------------------------------
     * Name:    inodeToBlockGroup
     * Type:    Function
     * Purpose: ...
     * Input:   u32 inode_number, containing ...
     * Output:  u32....
    ----------------------------------------------------------------------------------------------*/
    u32 ext2::inodeToBlockGroup(u32 inode_number)
    {
        return (inode_number - 1) / superBlock.s_inodes_per_group;
    }
    
    /*----------------------------------------------------------------------------------------------
     * Name:    inodeBlockGroupIndex
     * Type:    Function
     * Purpose: ...
     * Input:   u32 inode_number, containing ...
     * Output:  u32....
    ----------------------------------------------------------------------------------------------*/
    u32 ext2::inodeBlockGroupIndex(u32 inode_number)
    {
        return (inode_number - 1) % superBlock.s_inodes_per_group;
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
        return block_number < superBlock.s_blocks_count ?
               bootSector.partitionTable[0].firstSector * VDI_SECTOR_SIZE + 
                  block_number * (EXT2_BLOCK_BASE_SIZE << superBlock.s_log_block_size) :
               -1;
    }
    
    
    /*----------------------------------------------------------------------------------------------
     * Name:    inodeToOffset
     * Type:    Function
     * Purpose: ...
     * Input:   u32 inode_number, containing ...
     * Output:  off_t....
    ----------------------------------------------------------------------------------------------*/
    off_t ext2::inodeToOffset(u32 inode_number)
    {
        u32 inodeBlockGroup = inodeToBlockGroup(inode_number);
        u32 inodeIndex = inodeBlockGroupIndex(inode_number);
        //u32 inodeContainingBlock = (inodeIndex * superBlock.s_inode_size) / (EXT2_BLOCK_BASE_SIZE << superBlock.s_log_block_size);
        return
            //bootSector.partitionTable[0].firstSector * VDI_SECTOR_SIZE +
            blockToOffset(bgdTable[inodeBlockGroup].bg_inode_table) + inodeIndex * superBlock.s_inode_size;
//        return 0;
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
     * Input:   ext2_dir_entry* dir_entry, containing pointer to a directory object.
     * Output:  Nothing.
    ----------------------------------------------------------------------------------------------*/
    void ext2::print_dir_entry(ext2_dir_entry& dir_entry, bool verbose)
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
        cout << "Inodes count: " << superBlock.s_inodes_count << endl;
        cout << "Blocks count: " << superBlock.s_blocks_count << endl;
        cout << "Reserved blocks count: " << superBlock.s_r_blocks_count << endl;
        cout << "Free blocks count: " << superBlock.s_free_blocks_count << endl;
        cout << "Free inodes count: " << superBlock.s_free_inodes_count << endl;
        cout << "First data block: " << superBlock.s_first_data_block << endl;
        cout << "Block size: " << (EXT2_BLOCK_BASE_SIZE << superBlock.s_log_block_size) << endl;
        cout << "Fragment size: " << (EXT2_FRAG_BASE_SIZE << superBlock.s_log_frag_size) << endl;
        cout << "# blocks per group: " << superBlock.s_blocks_per_group << endl;
        cout << "# fragments per group: " << superBlock.s_frags_per_group << endl;
        cout << "# inodes per group: " << superBlock.s_inodes_per_group << endl;
        cout << "Mount time: " << superBlock.s_mtime << endl;
        cout << "Write time: " << superBlock.s_wtime << endl;
        cout << "Mount count: " << superBlock.s_mnt_count << endl;
        cout << "Maximal mount count: " << superBlock.s_max_mnt_count << endl;
        cout << "Magic signature: " << hex << superBlock.s_magic << dec << endl;
        cout << "File system state: " << superBlock.s_state << endl;
        cout << "Error behaviour: " << superBlock.s_errors << endl;
        cout << "Minor revision level: " << superBlock.s_minor_rev_level << endl;
        cout << "Time of last check: " << superBlock.s_lastcheck << endl;
        cout << "Check interval: " << superBlock.s_checkinterval << endl;
        cout << "Creator OS: " << superBlock.s_creator_os << endl;
        cout << "Revision level: " << superBlock.s_rev_level << endl;
        cout << "Default reserved UID: " << superBlock.s_def_resuid << endl;
        cout << "Default reserved GID: " << superBlock.s_def_resgid << endl;
        
        cout << "\nFirst non-reserved inode: " << superBlock.s_first_ino << endl;
        cout << "inode structure size: " << superBlock.s_inode_size << endl;
        cout << "Block group number of superblock: " << superBlock.s_block_group_nr << endl;
        cout << "Compatible feature set: " << superBlock.s_feature_compat << endl;
        cout << "Incompatible feature set: " << superBlock.s_feature_incompat << endl;
        cout << "Read-only compatible feature set: " << superBlock.s_feature_ro_compat << endl;
        cout << "Volume UUID: " << superBlock.s_uuid << endl;
        cout << "Volume Name: " << superBlock.s_volume_name << endl;
        cout << "Directory where last mounted: " << superBlock.s_last_mounted << endl;
        cout << "Compression algorithm usage bitmap: " << superBlock.s_algorithm_usage_bitmap << endl;
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
     *
     * @TODO    Verify that it's ok to read just from i_block[0] for a directory inode. <- it's not
    ----------------------------------------------------------------------------------------------*/
    vector<ext2::ext2_dir_entry> ext2::parse_directory_inode(ext2_inode inode)
    {
        vector<ext2_dir_entry> to_return;
        u32 cursor = 0;
        s8 name_buffer[256];
        u8* inode_buffer = nullptr;
        
        // Attempt to allocate and then verify memory for the inode buffer.
        inode_buffer = new u8[inode.i_size];
        if (inode_buffer == nullptr)
        {
            cout << "Error allocating directory inode buffer.";
            throw;
        }
        
        // Set the offset to the beginning of the block referenced by the inode.
        vdi->vdiSeek(blockToOffset(inode.i_block[0]), SEEK_SET);
        
        // Read the contents of the block into memory, based on the given size.
        vdi->vdiRead(inode_buffer, inode.i_size);
        
        // Iterate through the inode buffer, reading the directory entry records.
        while (cursor < inode.i_size)
        {
            // Add a new ext2_dir_entry to the back of the vector.
            to_return.emplace_back();
            
            // Read inode, record length, name length, and file type.
            memcpy(&(to_return.back()), &(inode_buffer[cursor]), EXT2_DIR_BASE_SIZE);
            cursor += EXT2_DIR_BASE_SIZE;
            
            // Read the name into the buffer.
            memcpy(name_buffer, &(inode_buffer[cursor]), to_return.back().name_len);
            cursor += to_return.back().name_len;
            
            // Add a null to the end of the characters read in so it becomes a C-style string.
            name_buffer[to_return.back().name_len] = '\0';
            
            // Store the name in the record.
            to_return.back().name.assign(name_buffer);
            
            // Set the offset to the next record.
            if (EXT2_DIR_BASE_SIZE + to_return.back().name_len < to_return.back().rec_len)
                cursor += to_return.back().rec_len - EXT2_DIR_BASE_SIZE - to_return.back().name_len;
            
            cout << "\ndebug::ext2::ext2_dir_entry\n";
            print_dir_entry(to_return.back());
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
     * Input:   u32 inode, containing an inode number.
     * Output:  vector<ext2_dir_entry>, contains a vector holding the contents of the inode.
     *
     * @TODO    Verify that it's ok to read just from i_block[0] for a directory inode.
    ----------------------------------------------------------------------------------------------*/
    vector<ext2::ext2_dir_entry> ext2::parse_directory_inode(u32 inode)
    {
        return parse_directory_inode(readInode(inode));
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
        if (inode >= superBlock.s_inodes_count)
        {
            cout << "inode out of bounds\n";
            throw;
        }
        
        ext2_inode toReturn;
        
        vdi->vdiSeek(inodeToOffset(inode), SEEK_SET);
        vdi->vdiRead(&toReturn, sizeof(ext2_inode));
        
        return toReturn;
    }
    

    /*----------------------------------------------------------------------------------------------
     * Name:    dir_entry_exists
     * Type:    Function
     * Purpose: Small function that verifies a path exists.
     * Input:   const string & path_to_check, containing the path to verify.
     * Output:  vector<ext2_dir_entry>, containing ext2_dir_entry objects representing the path to
     *          the desired folder if it is found, or containing nothing if not found.
    ----------------------------------------------------------------------------------------------*/
    vector<ext2::ext2_dir_entry> ext2::dir_entry_exists(const string & path_to_check){
        // Tokenize the path that will be checked.
        vector<string> path_tokens = utility::tokenize(path_to_check, DELIMITER_FSLASH);
        
        vector<ext2_dir_entry> to_return;
        
        // Check if the given path is relative or absolute.
        if(path_to_check[0] == '/') {
            // Absolute path given.
            to_return.push_back(pwd.front());
        }
        else {
            // Relative path given.
            to_return = pwd;
        }
        
        vector<ext2_dir_entry> current_dir_parse;
        bool found_dir = true;
        
        // Loop through the path_tokens vector, following the user's desired path sequentially.
        for (u32 i = 0; found_dir == true && i < path_tokens.size(); i++) {
            // Parse the current inode for its directory structure.
            current_dir_parse = parse_directory_inode(to_return.back().inode);
            
            // Reset the found directory flag on each iteration.
            found_dir = false;
            
            // Loop through the parsed directory listing to attempt to find a match.
            for (u32 j = 0; j < current_dir_parse.size(); j++) {
                // First, check if the directory entry is a folder.
                if (current_dir_parse[j].file_type == 2) {
                    // Then check if the names match.
                    if (path_tokens[i] == current_dir_parse[j].name) {
                        // If the directory entry is a folder and the names match, handle the
                        // different scenarios.
                        if (path_tokens[i] == ".") {
                            // Intentionally do nothing.
                        }
                        else if (path_tokens[i] == "..") {
                            // Go up one level unless already at the filesystem root.
                            if (to_return.size() > 1) {
                                to_return.pop_back();
                            }
                        }
                        else {
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
            to_return.clear();
        
        // Return the path to the folder.
        return to_return;
    } 
    
    
    void ext2::debug_dump_pwd_inode()
    {
        ext2_inode temp = readInode(pwd.back().inode);
        print_inode(&temp);
    }
    
    
    void ext2::print_block(u32 block_to_dump, bool text = true)
    {
        u8 * raw_block = new u8[EXT2_BLOCK_BASE_SIZE << superBlock.s_log_block_size];
        
        vdi->vdiSeek(blockToOffset(block_to_dump), SEEK_SET);
        vdi->vdiRead(raw_block, (EXT2_BLOCK_BASE_SIZE << superBlock.s_log_block_size));
        
        cout << "ext2::print_block\n";
        
        if (!text)
        {
            for (u32 i = 0; i < (EXT2_BLOCK_BASE_SIZE << superBlock.s_log_block_size); i++)
            {
                cout << hex << setw(2) << setfill('0') << (int)raw_block[i] << ' ';
            }
        }
        else
        {
            for (u32 i = 0; i < (EXT2_BLOCK_BASE_SIZE << superBlock.s_log_block_size); i++)
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
} // namespace vdi_explorer