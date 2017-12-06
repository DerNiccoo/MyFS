//
//  myfs.cpp
//  myfs
//
//  Created by Oliver Waldhorst on 02.08.17.
//  Copyright © 2017 Oliver Waldhorst. All rights reserved.
//

// Global
#include <iostream>
#include <cstring>
#include <cmath>

// Project specific
#include "myfs.h"
#include "myfs-info.h"

using namespace std;

MyFS* MyFS::_instance = NULL;

#define RETURN_ERRNO(x) (x) == 0 ? 0 : -errno

MyFS* MyFS::Instance() {
    if(_instance == NULL) {
        _instance = new MyFS();
    }
    return _instance;
}


MyFS::MyFS() {
}
MyFS::~MyFS() {
}


int MyFS::fuseGetattr(const char *path, struct stat *st) {
    LOGM();

	if ( strcmp( path, "/" ) == 0 )
	{
		st->st_uid = getuid(); // The owner of the file/directory is the user who mounted the filesystem
		st->st_gid = getgid(); // The group of the file/directory is the same as the group of the user who mounted the filesystem
		st->st_atime = time( NULL ); // The last "a"ccess of the file/directory is right now
		st->st_mtime = time( NULL ); // The last "m"odification of the file/directory is right now
		st->st_mode = S_IFDIR | 0444;
		st->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is here: http://unix.stackexchange.com/a/101536
		return 0;
	}
	else
	{
		uint32_t pointer = -1;
		char copy[512];	//Max. größe 512 und auch immer 512 groß
		Inode* node = (Inode*)copy;
		path++;
		while((pointer = MyFSMgr::instance()->readNextRootPointer(pointer)) != 0){
			MyFSMgr::BDInstance()->read(pointer, (char*)node);

			if (strcmp(node->fileName, path) == 0){
				st->st_nlink = 1;
				st->st_size = node->size;
				st->st_gid = node->gid;
				st->st_uid = node->uid;
				st->st_mode = node->mode;
				st->st_atim.tv_sec = node->atim;
				st->st_mtim.tv_sec = node->mtim;
				st->st_ctim.tv_sec = node->ctim;
				return 0;
			}
		}
	}

    return RETURN_ERRNO(ENOENT);
}

int MyFS::fuseReadlink(const char *path, char *link, size_t size) {
    LOGM();
    return 0;
}

int MyFS::fuseMknod(const char *path, mode_t mode, dev_t dev) {
    LOGM();
    return 0;
}

int MyFS::fuseMkdir(const char *path, mode_t mode) {
    LOGM();
    return 0;
}

int MyFS::fuseUnlink(const char *path) {
    LOGM();

	uint32_t pointer = -1;
	char copy[512];
	Inode* node = (Inode*)copy;
	path++;
	while((pointer = MyFSMgr::instance()->readNextRootPointer(pointer)) != 0){
		MyFSMgr::BDInstance()->read(pointer, (char*)node);

		if (strcmp(node->fileName, path) == 0){
			MyFSMgr::instance()->removeFile(pointer);
			return 0;
		}
	}
    return 0;	//TODO: ErrorCode
}

int MyFS::fuseRmdir(const char *path) {
    LOGM();
    return 0;
}

int MyFS::fuseSymlink(const char *path, const char *link) {
    LOGM();
    return 0;
}

int MyFS::fuseRename(const char *path, const char *newpath) {
    LOGM();
    return 0;
}

int MyFS::fuseLink(const char *path, const char *newpath) {
    LOGM();
    return 0;
}

int MyFS::fuseChmod(const char *path, mode_t mode) {
    LOGM();
    return 0;
}

int MyFS::fuseChown(const char *path, uid_t uid, gid_t gid) {
    LOGM();
    return 0;
}

int MyFS::fuseTruncate(const char *path, off_t newSize) {
    LOGM();
    return 0;
}

int MyFS::fuseUtime(const char *path, struct utimbuf *ubuf) {
    LOGM();
    return 0;
}

int MyFS::fuseOpen(const char *path, struct fuse_file_info *fileInfo) {
    LOGM();
    return 0;
}

int MyFS::fuseRead(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo) {
    LOGM();

    uint32_t pointer = -1;
	char copy[BLOCK_SIZE];	//Max. größe 512 und auch immer 512 groß
	char read[BLOCK_SIZE];
	Inode* node = (Inode*)copy;
	path++;
	while((pointer = MyFSMgr::instance()->readNextRootPointer(pointer)) != 0){
		MyFSMgr::BDInstance()->read(pointer, (char*)node);

		if (strcmp(node->fileName, path) == 0){
			uint32_t pointer = node->pointer;
			uint32_t off = 0;
			do {
				MyFSMgr::BDInstance()->read(pointer, read);
				memcpy(buf + off, read, BLOCK_SIZE);
				off = off + BLOCK_SIZE;
			} while ((pointer = MyFSMgr::instance()->readFAT(pointer)) != UINT32_MAX);

			return node->size;
		}
	}

    return 0;	//TODO: ErrorCode
}

int MyFS::fuseWrite(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo) {
    LOGM();
    return 0;
}

int MyFS::fuseStatfs(const char *path, struct statvfs *statInfo) {
    LOGM();
    return 0;
}

int MyFS::fuseFlush(const char *path, struct fuse_file_info *fileInfo) {
    LOGM();
    return 0;
}

int MyFS::fuseRelease(const char *path, struct fuse_file_info *fileInfo) {
    LOGM();
    return 0;
}

int MyFS::fuseFsync(const char *path, int datasync, struct fuse_file_info *fi) {
    LOGM();
    return 0;
}

int MyFS::fuseSetxattr(const char *path, const char *name, const char *value, size_t size, int flags) {
    LOGM();
    return 0;
}

int MyFS::fuseGetxattr(const char *path, const char *name, char *value, size_t size) {
    LOGM();
    return 0;
}

int MyFS::fuseListxattr(const char *path, char *list, size_t size) {
    LOGM();
    return 0;
}

int MyFS::fuseRemovexattr(const char *path, const char *name) {
    LOGM();
    return 0;
}

int MyFS::fuseOpendir(const char *path, struct fuse_file_info *fileInfo) {
    LOGM();
    return 0;
}

int MyFS::fuseReaddir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fileInfo) {
    LOGM();

	filler( buf, ".", NULL, 0 ); // Current Directory
	filler( buf, "..", NULL, 0 ); // Parent Directory

	if ( strcmp( path, "/" ) == 0 ) // If the user is trying to show the files/directories of the root directory show the following
	{
		uint32_t pointer = -1;
		char copy[BLOCK_SIZE];
		Inode* node = (Inode*)copy;
		while((pointer = MyFSMgr::instance()->readNextRootPointer(pointer)) != 0){
			MyFSMgr::BDInstance()->read(pointer, (char*)node);
			filler(buf, node->fileName, NULL, 0);
		}

	} else {
		return RETURN_ERRNO(ENOTDIR);
	}

    return 0;
}

int MyFS::fuseReleasedir(const char *path, struct fuse_file_info *fileInfo) {
    LOGM();
    return 0;
}

int MyFS::fuseFsyncdir(const char *path, int datasync, struct fuse_file_info *fileInfo) {
    LOGM();
    return 0;
}

int MyFS::fuseInit(struct fuse_conn_info *conn) {

    // Initialize time based logging
    Logger::getLogger()->setTimeBasedLogging(true);
    if (Logger::getLogger()->setLogfile(((MyFsInfo*) fuse_get_context()->private_data)->logFile) != 0)
        return -1;
    
    // Get the container file name here:
    LOGF("Container file name: %s\n", ((MyFsInfo*) fuse_get_context()->private_data)->contFile);
    
    // TODO: Enter your code here!
    MyFSMgr::BDInstance()->open(((MyFsInfo*) fuse_get_context()->private_data)->contFile);
    
    return 0;
}

int MyFS::fuseTruncate(const char *path, off_t offset, struct fuse_file_info *fileInfo) {
    LOGM();
    return 0;
}

int MyFS::fuseCreate(const char *path, mode_t mode, struct fuse_file_info *fileInfo) {
    LOGM();
    return 0;
}

void MyFS::fuseDestroy() {
    LOGM();
}


