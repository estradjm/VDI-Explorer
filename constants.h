// constants.h
// Define some constants that repeatedly appear as magic numbers in the project.

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>

using namespace std;

const int BITS_PER_BYTE = 8;
const int BITS_PER_1K = 1024;
const int BITS_PER_4K = 4096;
const int BITS_PER_8K = 8192;

const int MBR_SIGNATURE = 0xaa55; // Magic number that serves as a signature to an MBR.
const int MBR_SIZE_OF_BOOTSTRAP_CODE = 0x1be; // Length in bytes of the MBR's bootstrap code.
const int MBR_NUMBER_OF_PARTITIONS = 4; // The number of partition entries in an MBR.

const int VDI_IMAGE_SIGNATURE = 0xbeda107f; // Magic number that serves as a signature to a VDI file.
const int VDI_SECTOR_SIZE = 512; // The size in bytes of a sector in a VDI. (constant or variable?)

const int EXT2_SUPERBLOCK_OFFSET = 1024; // The superblock is located 1024 bytes from the beginning of the volume.
const int EXT2_SUPERBLOCK_SIZE = 1024; // The superblock is 1024 bytes long.
const unsigned int EXT2_BLOCK_BASE_SIZE = 1024; // The base block size is 1024 bytes.
const unsigned int EXT2_FRAG_BASE_SIZE = 1024; // The base fragment size is 1024 bytes.

const int EXT2_BLOCK_POINTER_SIZE = 4; // The size of a block pointer in bytes.
const unsigned long long int EXT2_MAX_ABS_FILE_SIZE = 2199023255040; // (2^32-1)*512 => The absolute maximum file size allowed by the ext2 file system. (2 TiB)
const int EXT2_FILENAME_MAX_LENGTH = 255; // The max number of characters allowed in a filename in the ext2 file system.

const int EXT2_INODE_NBLOCKS_DIR = 12; // The number of direct block pointers in an inode.
const int EXT2_INODE_BLOCK_S_IND = EXT2_INODE_NBLOCKS_DIR; // Array location of the singly indirect block pointer in an inode.
const int EXT2_INODE_BLOCK_D_IND = EXT2_INODE_BLOCK_S_IND + 1; // Array location of the doubly indirect block pointer in an inode.
const int EXT2_INODE_BLOCK_T_IND = EXT2_INODE_BLOCK_D_IND + 1; // Array location of the triply indirect block pointer in an inode.
const int EXT2_INODE_NBLOCKS_TOT = EXT2_INODE_BLOCK_T_IND + 1; // Total number of blocks in an inode.

const int EXT2_INODE_TYPE_FIFO = 0x1000;
const int EXT2_INODE_TYPE_CHARDEV = 0x2000;
const int EXT2_INODE_TYPE_DIR = 0x4000;
const int EXT2_INODE_TYPE_BLOCKDEV = 0x6000;
const int EXT2_INODE_TYPE_FILE = 0x8000;
const int EXT2_INODE_TYPE_SYMLINK = 0xA000;
const int EXT2_INODE_TYPE_SOCKET = 0xC000;

const int EXT2_INODE_PERM_OTHER_EXECUTE = 0x001;
const int EXT2_INODE_PERM_OTHER_WRITE = 0x002;
const int EXT2_INODE_PERM_OTHER_READ = 0x004;
const int EXT2_INODE_PERM_GROUP_EXECUTE = 0x008;
const int EXT2_INODE_PERM_GROUP_WRITE = 0x010;
const int EXT2_INODE_PERM_GROUP_READ = 0x020;
const int EXT2_INODE_PERM_USER_EXECUTE = 0x040;
const int EXT2_INODE_PERM_USER_WRITE = 0x080;
const int EXT2_INODE_PERM_USER_READ = 0x100;
const int EXT2_INODE_PERM_STICKYBIT = 0x200;
const int EXT2_INODE_PERM_SETGID = 0x400;
const int EXT2_INODE_PERM_SETUID = 0x800;

const int EXT2_INODE_FLAGS_SECURE_DELETE = 0x00000001;
const int EXT2_INODE_FLAGS_KEEP_COPY_ON_DELETE = 0x00000002;
const int EXT2_INODE_FLAGS_FILE_COMPRESSION = 0x00000004;
const int EXT2_INODE_FLAGS_SYNC_UPDATES = 0x00000008;
const int EXT2_INODE_FLAGS_IMMUTABLE = 0x00000010;
const int EXT2_INODE_FLAGS_APPEND = 0x00000020;
const int EXT2_INODE_FLAGS_DO_NOT_DUMP = 0x00000040;
const int EXT2_INODE_FLAGS_LAST_ACCESS_NO_UPDATE = 0x00000080;
const int EXT2_INODE_FLAGS_HASH_INDEX_DIR = 0x00010000;
const int EXT2_INODE_FLAGS_AFS_DIR = 0x00020000;
const int EXT2_INODE_FLAGS_JOURNAL_FILE_DATA = 0x0004000;

const int EXT2_DIR_BASE_SIZE = 8; // The base size of an ext2_dir_entry structure.

const int EXT2_DIR_TYPE_UNKNOWN = 0;
const int EXT2_DIR_TYPE_FILE = 1;
const int EXT2_DIR_TYPE_DIR = 2;
const int EXT2_DIR_TYPE_CHARDEV = 3;
const int EXT2_DIR_TYPE_BLOCKDEV = 4;
const int EXT2_DIR_TYPE_FIFO = 5;
const int EXT2_DIR_TYPE_SOCKET = 6;
const int EXT2_DIR_TYPE_SYMLINK = 7;

const string DELIMITER_SPACE = " ";
const string DELIMITER_FSLASH = "/";
const string DELIMITER_DOT = ".";

const unsigned short EXT2_INODE_DEFAULT_UID = 1000; // Default user ID.  1000 = root
const unsigned short EXT2_INODE_DEFAULT_GID = 1000; // Default group ID.  1000 = root

#endif