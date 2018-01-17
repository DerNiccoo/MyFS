//
//  blockdevice.cpp
//  myfs
//
//  Created by Oliver Waldhorst on 09.10.17.
//  Copyright © 2017 Oliver Waldhorst. All rights reserved.
//

#include <cstdlib>
#include <cassert>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "macros.h"
#include "blockdevice.h"

/**
 * Initializes a new instance of the BlockDevice class with the given block size.
 *
 * @param blockSize The size of a single block.
 */
BlockDevice::BlockDevice(uint32_t blockSize) {
    assert(blockSize % 512 == 0);
    this->blockSize = blockSize;

    this->contFile = 0;
    this->size = 0;
}

void BlockDevice::resize(u_int32_t blockSize) {
    assert(blockSize % 512 == 0);
    this->blockSize = blockSize;
}

/**
 * Create the container file with the given path.
 * If the file does already exist it will be overwritten.
 *
 * @param path The absolute path of the container file.
 * @return Always 0.
 */
int BlockDevice::create(const char* path) {
    
    // Delete existing container file
    if (access( path, F_OK ) != -1) {
        if (remove( path ) != 0) {
            error("Error deleting existing container file!");
        } else {
            LOG("Existing container file successfully deleted.");
        }
    }
    
    // Create new container file
    contFile = ::open(path, O_EXCL | O_RDWR | O_CREAT, 0666);
    if (contFile < 0) {
        if (errno == EEXIST)
            error("Container file already exists!");
        else
            error("Unable to create container file!");
    }
    
    // Read file stats
    struct stat st;
    stat(path, &st);
    
    // Get file size
    off_t size = st.st_size;
    if (size > INT32_MAX)
        error("File is to large!");
    this->size = (uint32_t) size;
    
    return 0;
}

/**
 * Open the container file with the given path.
 *
 * @param path The absolute path of the container file.
 * @return Always 0.
 */
int BlockDevice::open(const char *path) {
    
    // Open Container file
    contFile = ::open(path, O_EXCL | O_RDWR);
    if (contFile < 0) {
        if (errno == ENOENT)
            error("Container file does not exist!");
        else
            error("Unknown error!");
    }
    
    // Read file stats
    struct stat st;
    stat(path, &st);
    
    // Get file size
    if (st.st_size > INT32_MAX)
        error("File is to large!");
    this->size = (uint32_t) st.st_size;
    
    return 0;
}

/**
 * Close the file descriptor FD.
 *
 * @return Always 0.
 */
int BlockDevice::close() {

    ::close(this->contFile);
    
    return 0;
}

/**
 * Read the data of a single Block from the BlockDevice.
 *
 * @param blockNo The block to read.
 * @param buffer out The read data.
 * @return Always 0.
 */
int BlockDevice::read(u_int32_t blockNo, char *buffer) {
    seekto(this->contFile, blockNo);
//    LOGF("read: %u\n",blockNo);
    readbuf(this->contFile, buffer, this->blockSize);
    return 0;
}

/**
 * Write the provided data to the specified block number.
 * Note that the size of data must be equal to the specified blockSize.
 *
 * @param blockNo The block where the data should be written to.
 * @param buffer A pointer to the data which should be written.
 * @return Always 0.
 */
int BlockDevice::write(u_int32_t blockNo, char *buffer) {
    seekto(this->contFile, blockNo);
    writebuf(this->contFile, buffer, this->blockSize);
    return 0;
}

