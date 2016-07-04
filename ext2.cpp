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
        cout << "\nBootSector struct: ";
        // cout << "\nBootstrap code: ";
        // for (int i = 0; i < 0x1be; i++)
        //     cout << hex << setfill('0') << setw(2) << (int)bootSector.unused0[i] << dec << " ";
        for (int i = 0; i < 4 /*number of partition tables*/; i++) {
            cout << "\nPartition Entry " << i+1 << ": ";
            cout << "\n  First Sector: " << bootSector.partitionTable[i].firstSector;
            cout << "\n  nSectors: " << bootSector.partitionTable[i].nSectors;
        }
        cout << "\nBoot Sector Signature: " << hex << bootSector.magic << dec << endl;
        // End debug info.
        
        // Read the superblock.
        vdi->vdiSeek(bootSector.partitionTable[0].firstSector * VDI_SECTOR_SIZE +
                     EXT2_SUPERBLOCK_OFFSET, SEEK_SET);
        vdi->vdiRead(&superBlock, sizeof(ext2_superblock));
        
        // Debug info.
        // cout << "\nSuper Block Raw:\n";
        // for (u32 i = 0; i < sizeof(ext2_superblock); i++)
        //     cout << hex << setfill('0') << setw(2) << (int)((u8 *)(&superBlock))[i] << " ";
        // cout << dec << "\n\n";
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
        // End debug info.
        
        ext2_inode temp = readInode(30481);
        
        // Debug info.
        print_inode(&temp);
        // End debug info.
        
        // u8 dummy = 0;
        // vdi->vdiSeek(blockToOffset(temp.i_block[0]), SEEK_SET);
        // vdi->vdiRead(&dummy, 1);
        
        auto temp2 = parse_directory_inode(temp);
        for (u32 i = 0; i < temp2.size(); i++)
            print_dir_entry(temp2[i]);
        
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
            (((inode->i_mode >> 12) << 12) == EXT2_INODE_TYPE_UNIXSOCK ? "Unix socket\n" : "");
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
            // cout << "\nDirectory Entry:\n";
            // cout << "Inode number: " << dir_entry->inode << endl;
            // cout << "Directory record length: " << dir_entry->rec_len << endl;
            // cout << "Name length: " << (int)(dir_entry->name_len) << endl;
            // cout << "Directory entry type: " <<
            //     (dir_entry->file_type == EXT2_DIR_TYPE_UNKNOWN ? "unknown\n" : "") <<
            //     (dir_entry->file_type == EXT2_DIR_TYPE_REGULAR ? "regular file\n" : "") <<
            //     (dir_entry->file_type == EXT2_DIR_TYPE_DIRECTORY ? "directory\n" : "") <<
            //     (dir_entry->file_type == EXT2_DIR_TYPE_CHARDEV ? "character device\n" : "") <<
            //     (dir_entry->file_type == EXT2_DIR_TYPE_BLOCKDEV ? "block device\n" : "") <<
            //     (dir_entry->file_type == EXT2_DIR_TYPE_FIFO ? "FIFO\n" : "") <<
            //     (dir_entry->file_type == EXT2_DIR_TYPE_SOCKET ? "socket\n" : "") <<
            //     (dir_entry->file_type == EXT2_DIR_TYPE_SYMLINK ? "symbolic link\n" : "");
            // cout << "Name: ";
            // for (u8 i = 0; i < (dir_entry->name_len); i++)
            //     cout << dir_entry->name[i];
            // cout << endl;
            cout << "\nDirectory Entry:\n";
            cout << "Inode number: " << dir_entry.inode << endl;
            cout << "Directory record length: " << dir_entry.rec_len << endl;
            cout << "Name length: " << (int)(dir_entry.name_len) << endl;
            cout << "Directory entry type: " <<
                (dir_entry.file_type == EXT2_DIR_TYPE_UNKNOWN ? "unknown\n" : "") <<
                (dir_entry.file_type == EXT2_DIR_TYPE_REGULAR ? "regular file\n" : "") <<
                (dir_entry.file_type == EXT2_DIR_TYPE_DIRECTORY ? "directory\n" : "") <<
                (dir_entry.file_type == EXT2_DIR_TYPE_CHARDEV ? "character device\n" : "") <<
                (dir_entry.file_type == EXT2_DIR_TYPE_BLOCKDEV ? "block device\n" : "") <<
                (dir_entry.file_type == EXT2_DIR_TYPE_FIFO ? "FIFO\n" : "") <<
                (dir_entry.file_type == EXT2_DIR_TYPE_SOCKET ? "socket\n" : "") <<
                (dir_entry.file_type == EXT2_DIR_TYPE_SYMLINK ? "symbolic link\n" : "");
            cout << "Name: " << dir_entry.name << endl;
        }
        else
        {
            cout << dir_entry.name << endl;
        }
    }
    
    
    /*----------------------------------------------------------------------------------------------
     * Name:    parse_directory_inode
     * Type:    Function
     * Purpose: ....
     * Input:   ext2_inode inode, containing pointer to an inode object.
     * Output:  vector<ext2_dir_entry>, contains a vector holding the contents of the inode.
     *
     * @TODO    Verify that it's ok to read just from i_block[0] for a directory inode.
    ----------------------------------------------------------------------------------------------*/
    // void ext2::parse_directory_inode(ext2_inode inode)
    std::vector<ext2::ext2_dir_entry> ext2::parse_directory_inode(ext2_inode inode)
    {
        vector<ext2_dir_entry> toReturn;
        u32 cursor = 0;
        s8 temp_name[256];
        
        // Set the offset to the beginning of the block referenced by the inode.
        vdi->vdiSeek(blockToOffset(inode.i_block[0]), SEEK_SET);

        while (cursor < inode.i_size)
        {
            // Add a new ext2_dir_entry to the back of the vector.
            toReturn.emplace_back();
            
            // read inode, record length, name length, and file type.
            vdi->vdiRead(&(toReturn.back()), 4 + 2 + 1 + 1);
            
            // read the name
            vdi->vdiRead(temp_name, toReturn.back().name_len);
            temp_name[toReturn.back().name_len] = '\0';
            toReturn.back().name.assign(temp_name);
            
            // set the offset to the next record
            if (EXT2_DIR_BASE_SIZE + toReturn.back().name_len < toReturn.back().rec_len)
                vdi->vdiSeek(toReturn.back().rec_len - EXT2_DIR_BASE_SIZE - toReturn.back().name_len, SEEK_CUR);
            
            // update the cursor
            cursor += toReturn.back().rec_len;
            
            // Debug info.
            // if (toReturn.back().inode)
            //     print_dir_entry(toReturn.back());
        }
        
        return toReturn;
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
} // namespace vdi_explorer