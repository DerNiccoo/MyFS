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

BlockDevice::BlockDevice(uint32_t blockSize) {
    assert(blockSize % 512 == 0);
    this->blockSize= blockSize;
}

void BlockDevice::resize(u_int32_t blockSize) {
    assert(blockSize % 512 == 0);
    this->blockSize= blockSize;
}

int BlockDevice::create(const char *path) {
    
    // TODO: handle block devices
    
    // Open Container file
    contFile = ::open(path, O_EXCL | O_RDWR | O_CREAT, 0666);
    if (contFile < 0) {
        if (errno == EEXIST)
            error("container file already exists");
        else
            error("unable to create container file");
    }
    
    // read file stats
    struct stat st;
    stat(path, &st);
    
    // get file size
    off_t size = st.st_size;
    if(size > INT32_MAX)
        error("file to large");
    this->size= (uint32_t) size;
    
    return 0;
}

int BlockDevice::open(const char *path) {
    
    // TODO: handle block devices
    
    // Open Container file
    contFile = ::open(path, O_EXCL | O_RDWR);
    if (contFile < 0) {
        if (errno == ENOENT)
            error("container file does not exists");
        else
            error("unknown error");
    }
    
    // read file stats
    struct stat st;
    stat(path, &st);
    
    // get file size
    if(st.st_size > INT32_MAX)
        error("file to large");
    this->size= (uint32_t) st.st_size;
    
    return 0;
}

int BlockDevice::close() {
    
    ::close(this->contFile);
    
    return 0;
}

int BlockDevice::read(u_int32_t blockNo, char *buffer) {
    seekto(this->contFile, blockNo);
    readbuf(this->contFile, buffer, this->blockSize);
    return 0;
}

int BlockDevice::write(u_int32_t blockNo, char *buffer) {
	seekto(this->contFile, blockNo);
	writebuf(this->contFile, buffer, this->blockSize);
    return 0;
}

