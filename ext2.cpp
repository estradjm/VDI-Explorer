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
        cout << "\nBootstrap code: ";
        for (int i = 0; i < 0x1be; i++)
            cout << hex << setfill('0') << setw(2) << (int)bootSector.unused0[i] << dec << " ";
        for (int i = 0; i < 4 /*number of partition tables*/; i++) {
            cout << "\nPartition Entry " << i+1 << ": ";
            cout << "\n  First Sector: " << bootSector.partitionTable[i].firstSector;
            cout << "\n  nSectors: " << bootSector.partitionTable[i].nSectors;
        }
        cout << "\nBoot Sector Signature: " << hex << bootSector.magic << dec << endl;
        // End debug info.
        
        // Read the superblock.
        vdi->vdiSeek(bootSector.partitionTable[0].firstSector * 512 /*sector size*/ +
                     1024 /* superblock size */, SEEK_SET);
        vdi->vdiRead(&superBlock, 1024);
        
        // Debug info.
        cout << "\nSuper Block Raw:\n";
        for (int i = 0; i < 1024; i++)
            cout << hex << setfill('0') << setw(2) << (int)((u8 *)(&superBlock))[i] << " ";
        cout << dec << "\n\n";
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
        cout << "Magic signature: " << hex << superBlock.s_magic << dec << endl;
        // End debug info.
        
        // Calculate and verify the number of block groups in the partition.
        u32 nBlockGroupCalc1, nBlockGroupCalc2;
        nBlockGroupCalc1 = (float)superBlock.s_blocks_count / superBlock.s_blocks_per_group + 0.5;
        nBlockGroupCalc2 = (float)superBlock.s_inodes_count / superBlock.s_inodes_per_group + 0.5;
        numBlockGroups = nBlockGroupCalc1;
        if (nBlockGroupCalc1 != nBlockGroupCalc2)
        {
            cout << "Block group calculation mismatch.";
            throw;
        }
        
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
                              sizeof(bootSector) + 512 + sizeof(superBlock);
        
        // Read the block group descriptor table into memory.
        vdi->vdiSeek(bgdTableStart, SEEK_SET);
        vdi->vdiRead(bgdTable, sizeof(ext2_block_group_desc) * numBlockGroups);
        // bgdTable doesn't need the "address of" operator (&) because it's already a pointer type.
    }
    
    ext2::~ext2()
    {
        // Delete the block descriptor table.
        if (bgdTable != nullptr)
            delete[] bgdTable;
    }
    
    // Needed?
    u32 ext2::offsetToBlock(off_t offset)
    {
        return offset / (EXT2_BLOCK_BASE_SIZE << superBlock.s_log_block_size);
    }
    
    u32 ext2::inodeToBlockGroup(u32 inode_number)
    {
        return (inode_number - 1) / superBlock.s_inodes_per_group;
    }
    
    u32 ext2::inodeBlockGroupIndex(u32 inode_number)
    {
        return (inode_number - 1) % superBlock.s_inodes_per_group;
    }
    
    off_t ext2::blockToOffset(u32 block_number)
    {
        return block_number < superBlock.s_blocks_count ?
               block_number * (EXT2_BLOCK_BASE_SIZE << superBlock.s_log_block_size) :
               -1;
    }
} // namespace vdi_explorer