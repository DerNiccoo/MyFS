//
//  mk.myfs.cpp
//  myfs
//
//  Created by Oliver Waldhorst on 07.09.17.
//  Copyright © 2017 Oliver Waldhorst. All rights reserved.
//

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
#include "MyFSMgr.h"

using namespace std;

// START # NOT IN USE
uint32_t readFAT(uint32_t blockPointer);
void addDateiSb();
Inode readNode(uint32_t nodePointer);

BlockDevice* _bd;

uint32_t readFAT(uint32_t blockPointer) {
    blockPointer -= DATA_START;
    int offsetBlockNR = blockPointer / 128;
    int offsetBlockPos = blockPointer % 128;

    char read[BLOCK_SIZE];
    _bd->read(FAT_START + offsetBlockNR, read); //FAT Start + Offset des Blocks

    uint32_t ergebnis = 0;
    uint32_t k = 2147483648;
    for (int i = (4 * offsetBlockPos); i < (4 * (offsetBlockPos + 1)); i++){
        ergebnis += ((unsigned short)read[i] / 65535) * k;
        k/=2;
    }

    return ergebnis;
}

void addDateiSb() {
    char copy[BLOCK_SIZE];
    Superblock* sb= (Superblock*) copy;
    _bd->read(0,(char*)sb);
    sb->Files = sb->Files+1;
    _bd->write(0,(char*)sb);
}

Inode readNode(uint32_t nodePointer) {
    Inode node;
    uint32_t currentPointer = 0;

    for (unsigned int i = NODE_START; i < NODE_ENDE; i++){
        if (nodePointer == currentPointer) { //Richtige Node
            _bd->read(i, (char*)&node);
            return node;
        }
        currentPointer++;
    }

    return node;
}
// END # NOT IN USE

/**
 * Initial main method of MyFS mkfs.
 *
 * @param argc The argument count as integer.
 * @param argv All passed arguments as string array.
 * @return 0 on success, nonzero on failure.
 */
int main(int argc, char* argv[]) {

    // Validate arguments
    if (argc < 3) {
        fprintf(stderr, "Usage: %s containerfile.bin inputfile1 [inputfile2 ...]\n", argv[0]);
        return (EXIT_FAILURE);
    }

    char containerFilePath[1024];
    MyFSMgr::Instance()->GetAbsPath(argv[1], containerFilePath);

    // Initialize block device
    MyFSMgr::Instance()->BDInstance()->create(containerFilePath);
    MyFSMgr::Instance()->FillBlocks(0, 16);
    MyFSMgr::Instance()->WriteSuperBlock();

    // Copy input files into our container file
    for (int i = 2; i < argc; i++) {
        if (MyFSMgr::Instance()->ImportFile(argv[i]) == -1)
            cout << "Duplicate file name!" << endl;
    }

    MyFSMgr::Instance()->BDInstance()->close();
    return (EXIT_SUCCESS);
}
