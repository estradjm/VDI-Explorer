// constants.h
// Define some constants that repeatedly appear as magic numbers in the project.

#ifndef CONSTANTS_H
#define CONSTANTS_H

const int BITS_IN_BYTE = 8;
const int BITS_IN_1K = 1024;
const int BITS_IN_4K = 4096;
const int BITS_IN_8K = 8192;

const int MBR_SIGNATURE = 0xaa55; // Magic number that serves as a signature to an MBR.
const int MBR_SIZE_OF_BOOTSTRAP_CODE = 0x1be; // Length in bytes of the MBR's bootstrap code.
const int MBR_NUMBER_OF_PARTITIONS = 4; // The number of partition entries in an MBR.

const int VDI_IMAGE_SIGNATURE = 0xbeda107f; // Magic number that serves as a signature to a VDI file.
const int VDI_SECTOR_SIZE = 512; // The size in bytes of a sector in a VDI. (constant or variable?)

const int EXT2_SUPERBLOCK_OFFSET = 1024; // The superblock is located 1024 bytes from the beginning of the volume.
const int EXT2_SUPERBLOCK_SIZE = 1024; // The superblock is 1024 bytes long.
const int EXT2_BLOCK_BASE_SIZE = 1024; // The base block size is 1024 bytes.
const int EXT2_FRAG_BASE_SIZE = 1024; // The base fragment size is 1024 bytes.

const int EXT2_INODE_NBLOCKS_DIR = 12; // The number of direct block pointers in an inode.
const int EXT2_INODE_BLOCK_S_IND = EXT2_INODE_NBLOCKS_DIR; // Array location of the singly indirect block pointer in an inode.
const int EXT2_INODE_BLOCK_D_IND = EXT2_INODE_BLOCK_S_IND + 1; // Array location of the doubly indirect block pointer in an inode.
const int EXT2_INODE_BLOCK_T_IND = EXT2_INODE_BLOCK_D_IND + 1; // Array location of the triply indirect block pointer in an inode.
const int EXT2_INODE_NBLOCKS_TOT = EXT2_INODE_BLOCK_T_IND + 1; // Total number of blocks in an inode.

#endif