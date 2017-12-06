/*
 * MyFSMgr.h
 *
 *  Created on: Dec 3, 2017
 *      Author: FireEmerald
 */

#ifndef MYFSMGR_H_
#define MYFSMGR_H_

// Global
#include <iostream>
#include <string>
#include <sys/stat.h>

// TODO Chris: Only used by GetAbsPath, could be replaced.
#include <unistd.h>
#include <cstring>

// Project specific
#include "blockdevice.h"
#include "Logger.h"

// Definitions
static int const NAME_LENGTH      = 255; // Max. length of a filename
static int const BLOCK_SIZE       = 512; // Logical Block Size
static int const NUM_DIR_ENTRIES  =  64; // Max. directory entries
static int const NUM_OPEN_FILES   =  64; // Max. open files per MyFS container file

static uint32_t const FAT_ENDE    =  3;
static uint32_t const FAT_START   =  1;
static uint32_t const NODE_START  =  4;
static uint32_t const NODE_ENDE   =  9;
static uint32_t const ROOT_BLOCK  = 10;
static uint32_t const DATA_START  = 11;
static uint32_t const MAX_UINT    = -1;
static uint32_t const SIZE        = -1;

struct Superblock {
    uint32_t Size;
    uint32_t PointerFat;
    uint32_t PointerNode;
    uint32_t PointerData;
    char Files;
};

struct Inode {
    char fileName[NAME_LENGTH]; // Max. 255 bytes (FileName + 24 bytes) (2 for the length)
    uint32_t size;              // 4 byte
    short gid;                  // 2 byte
    short uid;                  // 2 byte
    short mode;                 // 2 byte
    uint32_t atim;              // 4 byte
    uint32_t mtim;              // 4 byte
    uint32_t ctim;              // 4 byte
    uint32_t pointer;           // 4 byte
};

struct DataBuffer {
    uint32_t blockNumber;
    uint16_t dataPointer;
    struct Inode node;
    char data[512];
};

/**
 * Provides methods to handle MyFS.
 */
class MyFSMgr {
private:
    static MyFSMgr* _instance;
    static BlockDevice* _blockDevice;

    MyFSMgr();
    virtual ~MyFSMgr();

    // Operations
    int rootPointerCount();
    uint32_t findNextFreeBlock();
    Inode* readNode(uint32_t nodePointer);

    void createInode(char* path, uint32_t blockPointer);
    void writeInode(Inode* node);
    void writeRootPointer(uint32_t newPointer);
    void setFATBlockPointer(uint32_t blockPointer, uint32_t nextPointer);
    void removeFatPointer(uint32_t delPointer);
    void removeRootPointer(uint32_t delPointer);

public:
    static MyFSMgr* instance();
    static BlockDevice* BDInstance();

    void getAbsPath(char* arg, char* path);

    // BlockDevice
    void fillBlocks(uint32_t startBlockIndex, uint32_t endBlockIndex);
    void writeSuperBlock();

    // Operations
    int importFile(char* path);
    bool fileExists(char* path);
    void removeFile(uint32_t nodePointer);
    uint32_t readNextRootPointer(uint32_t position);
    uint32_t readFAT(uint32_t blockPointer);
};

#endif /* MYFSMGR_H_ */
