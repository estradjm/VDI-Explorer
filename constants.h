// constants.h
// Define some constants that repeatedly appear as magic numbers in the project.

#ifndef CONSTANTS_H
#define CONSTANTS_H

const int BITS_IN_BYTE = 8;
const int BITS_IN_1K = 1024;
const int BITS_IN_4K = 4096;
const int BITS_IN_8K = 8192;

const int MBR_SIGNATURE = 0xaa55; // Magic number that acts as a signature to an MBR.
const int MBR_SIZE_OF_BOOTSTRAP_CODE = 0x1be; // Length in bytes of the MBR's bootstrap code.
const int MBR_NUMBER_OF_PARTITIONS = 4; // The number of partition entries in an MBR.

#endif