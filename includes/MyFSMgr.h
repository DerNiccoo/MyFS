/*
 * MyFSMgr.h
 *
 *  Created on: Dec 3, 2017
 *      Author: FireEmerald
 */

#ifndef MYFSMGR_H_
#define MYFSMGR_H_

#include <iostream>
#include <string>
#include <sys/stat.h>

#include <unistd.h>
#include <cstring>

#include "blockdevice.h"

// Definitions
static int const NAME_LENGTH      = 255;
static int const BLOCK_SIZE       = 512;
static int const NUM_DIR_ENTRIES  =  64;
static int const NUM_OPEN_FILES   =  64;

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
    char fileName[NAME_LENGTH]; // max 255 bytes (FileName + 24 bytes) (2 for the length)
    uint32_t size;              // 4 byte
    short gid;                  // 2 byte
    short uid;                  // 2 byte
    short mode;                 // 2 byte
    uint32_t atim;              // 4 byte
    uint32_t mtim;              // 4 byte
    uint32_t ctim;              // 4 byte
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
    int RootPointerCount();
    uint32_t ReadNextRootPointer(uint32_t position);
    uint32_t FindNextFreeBlock();

    void CreateInode(char* path, uint32_t blockPointer);
    void WriteInode(Inode* node);
    void WriteRootPointer(uint32_t newPointer);
    void SetFATBlockPointer(uint32_t blockPointer, uint32_t nextPointer);

public:
    static MyFSMgr* Instance();
    static BlockDevice* BDInstance();

    void GetAbsPath(char* arg, char* path);

    // BlockDevice
    void FillBlocks(uint32_t startBlockIndex, uint32_t endBlockIndex);
    void WriteSuperBlock();

    // Operations
    int ImportFile(char* path);
    bool FileExists(char* path);
};

#endif /* MYFSMGR_H_ */
