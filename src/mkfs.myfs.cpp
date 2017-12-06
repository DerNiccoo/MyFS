//
//  mk.myfs.cpp
//  myfs
//
//  Created by Oliver Waldhorst on 07.09.17.
//  Copyright © 2017 Oliver Waldhorst. All rights reserved.
//

#include <errno.h>
#include <stdlib.h>
#include <iostream>

#include "myfs.h"
#include "blockdevice.h"
#include "macros.h"
#include "MyFSMgr.h"



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
    MyFSMgr::instance()->getAbsPath(argv[1], containerFilePath);

    LOG("Initialize block device...");
    MyFSMgr::instance()->BDInstance()->create(containerFilePath);
    MyFSMgr::instance()->fillBlocks(0, 16);
    MyFSMgr::instance()->writeSuperBlock();

    LOG("Copying files into our container file...");
    for (int i = 2; i < argc; i++) {
        if (MyFSMgr::instance()->importFile(argv[i]) == -1)
            LOG("Duplicate file name!");
    }

    MyFSMgr::instance()->BDInstance()->close();
    return (EXIT_SUCCESS);
}
