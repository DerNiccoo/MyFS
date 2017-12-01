//
//  mk.myfs.cpp
//  myfs
//
//  Created by Oliver Waldhorst on 07.09.17.
//  Copyright © 2017 Oliver Waldhorst. All rights reserved.
//

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <string.h>
#include <cstring>
#include <time.h>
#include <bitset>
#include <sstream>
#include <string>
#include <iostream>

#include "myfs.h"
#include "blockdevice.h"
#include "macros.h"

using namespace std;

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

struct Inode {
    char fileName[NAME_LENGTH]; // max 255 bytes (FileName + 24 bytes) (2 for the length)
    uint32_t size; // 4 byte
    short gid;     // 2 byte
    short uid;     // 2 byte
    short mode;    // 2 byte
    uint32_t atim; // 4 byte
    uint32_t mtim; // 4 byte
    uint32_t ctim; // 4 byte
};

struct Superblock {
    uint32_t Size;
    uint32_t PointerFat;
    uint32_t PointerNode;
    uint32_t PointerData;
    char Files;
};

void getPath(char *arg, char *path);
uint32_t findNextFreeBlock();
void setFATBlockPointer(uint32_t blockPointer, uint32_t nextPointer);
uint32_t readFAT(uint32_t blockPointer);
void fillBlocks(uint32_t startBlockIndex, uint32_t endBlockIndex);
int copyFile(char* path);
void createInode(char* path, uint32_t blockPointer);
void writeInode(char* data);
Inode readNode(uint32_t nodePointer);
int sumRootPointer();
bool checkDuplicate(char* path);
void writeRootPointer(uint32_t newPointer);

BlockDevice* bd;

/**
 * Determines the full path of a given file which must be inside the current working directory.
 *
 * @param arg The name of a file inside the current working directory.
 * @param path out The absolute path of the file.
 */
void getPath(char *arg, char *path){
    char currentWorkingDirectory[1024];

    // Get working directory and check if it couldn't be determined or SIZE was too small
    if (getcwd(currentWorkingDirectory, sizeof(currentWorkingDirectory)) == NULL)
       return;

    string absolutePath = (string)currentWorkingDirectory + "/" + (string)arg;
    strncpy(path, absolutePath.c_str(), 1024);
}

uint32_t readFAT(uint32_t blockPointer){
    blockPointer -= DATA_START;
    int offsetBlockNR = blockPointer / 128;
    int offsetBlockPos = blockPointer % 128;

    char read[BLOCK_SIZE];
    bd->read(FAT_START + offsetBlockNR, read); //FAT Start + Offset des Blocks

    uint32_t ergebnis = 0;
    uint32_t k = 2147483648;
    for (int i = (4 * offsetBlockPos); i < (4 * (offsetBlockPos + 1)); i++){
        ergebnis += ((unsigned short)read[i] / 65535) * k;
        k/=2;
    }

    return ergebnis;
}

void addDateiSb(){
    char copy[BLOCK_SIZE];
    Superblock* sb= (Superblock*) copy;
    bd->read(0,(char*)sb);
    sb->Files = sb->Files+1;
    bd->write(0,(char*)sb);
}

void writeSb() {
    char copy[BLOCK_SIZE];
    Superblock* sb = (Superblock*) copy;
    sb->Size = SIZE;
    sb->PointerData = DATA_START;
    sb->PointerFat = FAT_START;
    sb->PointerNode = NODE_START;

    bd->write(0, (char*)sb);
}

int sumRootPointer(){
    char read[BLOCK_SIZE];
    int sum = 0;

    bd->read(ROOT_BLOCK, read);

    for (int i = 0; i < BLOCK_SIZE; i+=4) {
        uint32_t pointer = read[i] << 24 | read[i+1] << 16 | read[i+2] << 8 | read[i+3];
        if (pointer != 0)
            sum++;
    }
    return sum;
}

void writeRootPointer(uint32_t newPointer) {
    char read[BLOCK_SIZE];

    bd->read(ROOT_BLOCK, read);
    for (int i = 0; i < BLOCK_SIZE; i+=4) {
        uint32_t pointer = read[i] << 24 | read[i+1] << 16 | read[i+2] << 8 | read[i+3];
        if (pointer == 0){
            char data[4];
            memcpy(data, &newPointer, 4);
            for (int k = 0; k < 4; k++) //damit die zahlen im normalen Style sind 0011 = 3 und nicht 1100 = 3
                read[i+3-k] = data[k];
            bd->write(ROOT_BLOCK, read);
            return;
        }
    }
}

// TODO Chris: Improve description...
/**
 * @param position -1 = Erster Treffer
 *                 PointerPosition = Es wird die NÄCHSTE zurückgegeben (damit lücken übersprungen werden, wenn man z.B. etwas löscht)
 * @return 0 = war der letzte Pointer sonst gibt es immer den nächsten Pointer
 */
uint32_t readNextRootPointer(uint32_t position){
    char read[BLOCK_SIZE];

    bd->read(ROOT_BLOCK, read);
    for (int i = 0; i < BLOCK_SIZE; i+=4) {
        uint32_t pointer = read[i] << 24 | read[i+1] << 16 | read[i+2] << 8 | read[i+3];

        if (pointer != 0 && position == MAX_UINT)
            return pointer;

        if (pointer == position)
            position = MAX_UINT;
    }

    return 0;
}

/**
 * Fill the specified blocks of the BlockDevice with 0.
 *
 * @param startBlockIndex Index of the first block to fill.
 * @param endBlockIndex Index of the last block to fill.
 */
void fillBlocks(uint32_t startBlockIndex, uint32_t endBlockIndex){
    char rawBlock[BLOCK_SIZE];
    for (int i = 0; i < BLOCK_SIZE; i++)
        rawBlock[i] = 0; // 255 = 1

    for (uint32_t i = startBlockIndex; i < endBlockIndex; i++)
        bd->write(i, rawBlock);
}

bool checkDuplicate(char* path) {
    Inode node;
    uint32_t position = MAX_UINT;
    char* newFileName = strtok(path, "/");
    char fileName[NAME_LENGTH];

    strcpy(fileName, newFileName);

    if (sumRootPointer() == 0) //Die erste Datei kann kein duplicate sein.
        return false;

    cout << fileName << endl;

    while((position = readNextRootPointer(position)) != 0){
        bd->read(position, (char*)&node);
        cout << fileName << " : " << node.fileName << endl;
        if (fileName == node.fileName)
            return true;
    }

    return false;
}

int copyFile(char* path){
    if (checkDuplicate(path))
        return -1;

    FILE *f = fopen(path, "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *string = (char *)malloc(fsize + 1);
    fread(string, fsize, 1, f);
    fclose(f);

    int position = 0;
    uint32_t blockPointer = findNextFreeBlock();
    uint32_t oldPointer;
    createInode(path, blockPointer);
    char data[BLOCK_SIZE];
    if (strlen(string) > BLOCK_SIZE) { //wir benötigen mehr als nur ein Block
        while((unsigned)(position * BLOCK_SIZE) < strlen(string)){
            for (int i = 0; i < BLOCK_SIZE; i++) //füllen der Daten zum schreiben
                data[i] = string[(position * BLOCK_SIZE) + i];

            bd->write(blockPointer, data);
            setFATBlockPointer(blockPointer, MAX_UINT); //block auf belegt setzen
            oldPointer = blockPointer;

            blockPointer = findNextFreeBlock(); //neuen Block holen
            setFATBlockPointer(oldPointer, blockPointer);    //verweiß setzen
            position++;
        }
    } else {    //izi. nur ein Block
        bd->write(blockPointer, string);
        setFATBlockPointer(blockPointer, MAX_UINT);
    }
    return 0;
}

void createInode(char* path, uint32_t blockPointer) {
    char copy[BLOCK_SIZE]; //Max. größe 512 und auch immer 512 groß
    Inode* node = (Inode*)copy;
    char* chars_array = strtok(path, "/");

    struct stat meta;
    stat(path, &meta);

    strcpy(node->fileName, chars_array);
    node->size = meta.st_size;
    node->gid = meta.st_gid;
    node->uid = meta.st_uid;
    node->mode = meta.st_mode;

    node->atim = meta.st_atim.tv_sec;
    node->mtim = meta.st_mtim.tv_sec;
    node->ctim = meta.st_ctim.tv_sec;

    writeInode((char*)node);
}

void writeInode(char* data) {
    char read[BLOCK_SIZE];

    for (uint32_t i = NODE_START; i < NODE_ENDE; i++){
        bd->read(i, read);
        if (read[0] == 0) { //Leerer Block
            bd->write(i, data);
            writeRootPointer(i);
            return;
        }
    }
}

Inode readNode(uint32_t nodePointer){
    Inode node;
    uint32_t currentPointer = 0;

    for (unsigned int i = NODE_START; i < NODE_ENDE; i++){
        if (nodePointer == currentPointer) { //Richtige Node
            bd->read(i, (char*)&node);
            return node;
        }
        currentPointer++;
    }

    return node;
}

void setFATBlockPointer(uint32_t blockPointer, uint32_t nextPointer){ //0x00000000 = FREI | 0xFFFFFFFF = BELEGT
    blockPointer -= DATA_START;
    int offsetBlockNR = blockPointer / 128;
    int offsetBlockPos = blockPointer % 128;

    char read[BLOCK_SIZE];
    bd->read(FAT_START + offsetBlockNR, read);

    char data[4];
    if (nextPointer != 0)
        memcpy(data, &nextPointer,sizeof(uint32_t));
    else {
        for (int i = 0; i < 4; i++)
            data[i] = 0;
    }

    int k = 0;
    for (int i = (4 * offsetBlockPos); i < (4 * (offsetBlockPos + 1)); i++){
        read[i] = data[k];
        k++;
    }

    bd->write(FAT_START + offsetBlockNR, read);
}

uint32_t findNextFreeBlock(){
    uint32_t ergebnis;
    char read[BLOCK_SIZE];
    for(uint32_t i = FAT_START; i < FAT_ENDE; i++){
        bd->read(i, read);
        for (int x = 0; x < 128; x++){ //Max. ints in einem Block
            ergebnis = 0;
            for(int m = (4 * x); m < (4 * (x+1)); m++){ //Bauen der Zahl
                ergebnis <<= 8;
                ergebnis |= read[m];
            }
            if (ergebnis == 0)//4294967295) //Wir einigten uns darauf das 0xFFFFFFFF = leer bedeutet
                return ((i - FAT_START) * 128) + x + DATA_START; //Alles damit wir in die Datenblöcke kommen
        }
    }
    cout << "wops, something went wrong!" << endl;
    return 0; //RETURN ERROR: FileSystem FULL!
}

/**
 * Initial main method of MyFS mkfs.
 *
 * @param argc The argument count as integer.
 * @param argv All passed arguments as string array.
 * @return 0 on success, nonzero on failure.
 */
int main(int argc, char *argv[]) {

    // Validate arguments
    if (argc < 3) {
        fprintf(stderr, "Usage: %s containerfile inputfile1 [inputfile2 ...]\n", argv[0]);
        return (EXIT_FAILURE);
    }

    char containerFilePath[1024];
    getPath(argv[1], containerFilePath);

    // Initialize block device
    bd = new BlockDevice(BLOCK_SIZE);
    bd->create(containerFilePath);
    fillBlocks(0, 16);
    writeSb();

    // Copy input files into our container file
    for (int i = 2; i < argc; i++) {
        if (copyFile(argv[i]) == -1)
            cout << "Duplicate file name!" << endl;
    }

    bd->close();
    return (EXIT_SUCCESS);
}
