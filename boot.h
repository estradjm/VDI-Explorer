#include "datatypes.h"

// the magic number found at the end of the master boot record
const u16 BOOT_SECTOR_MAGIC = 0xaa55;

// an entry in the partition table. the unused entries really hold information,
// but the information is antiquated and not useful for our purposes here
struct PartitionEntry {
  u8
    unused0[4],
    type,
    unused1[3];
  u32
    firstSector,
    nSectors;
};

// the master boot record.
// the __attribute__((packed)) tells GCC to not add hidden padding between
// fields. normally, padding is added to improve data access (it's faster if
// data starts on 4- or 8-byte boundaries). without packing, BootSector is
// 516 bytes, which is 4 too many.
//
// note: you want entry 0 in partitionTable.
struct __attribute__((packed)) BootSector {
  u8
    unused0[0x1be];
  PartitionEntry
    partitionTable[4];
  u16
    magic;
};

