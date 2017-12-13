//
//  mount.myfs.c
//  myfs
//
//  Created by Oliver Waldhorst on 02.08.17.
//  Copyright © 2017 Oliver Waldhorst. All rights reserved.
//

#include "wrap.h"

#include <fuse.h>
#include <stdio.h>

#include "myfs-info.h"

// The file system operations
struct fuse_operations myfs_oper;

/**
 * Initial main method of MyFS mount.
 *
 * @param argc The argument count as integer.
 * @param argv All passed arguments as string array.
 * @return 0 on success, nonzero on failure.
 */
int main(int argc, char *argv[]) {
    int fuse_stat;
    
    // Mapping of the wrapper methods
    myfs_oper.getattr = wrap_getattr;
    myfs_oper.readlink = wrap_readlink;
    myfs_oper.getdir = NULL;
    myfs_oper.mknod = wrap_mknod;
    myfs_oper.mkdir = wrap_mkdir;
    myfs_oper.unlink = wrap_unlink;
    myfs_oper.rmdir = wrap_rmdir;
    myfs_oper.symlink = wrap_symlink;
    myfs_oper.rename = wrap_rename;
    myfs_oper.link = wrap_link;
    myfs_oper.chmod = wrap_chmod;
    myfs_oper.chown = wrap_chown;
    myfs_oper.truncate = wrap_truncate;
    myfs_oper.utime = wrap_utime;
    myfs_oper.open = wrap_open;
    myfs_oper.read = wrap_read;
    myfs_oper.write = wrap_write;
    myfs_oper.statfs = wrap_statfs;
    myfs_oper.flush = wrap_flush;
    myfs_oper.release = wrap_release;
    myfs_oper.fsync = wrap_fsync;
    myfs_oper.setxattr = wrap_setxattr;
    myfs_oper.getxattr = wrap_getxattr;
    myfs_oper.listxattr = wrap_listxattr;
    myfs_oper.removexattr = wrap_removexattr;
    myfs_oper.opendir = wrap_opendir;
    myfs_oper.readdir = wrap_readdir;
    myfs_oper.releasedir = wrap_releasedir;
    myfs_oper.fsyncdir = wrap_fsyncdir;
    myfs_oper.init = wrap_init;
    
    // FsInfo will be used to pass information to fuse functions
    struct MyFsInfo *FsInfo;
    FsInfo = malloc(sizeof(struct MyFsInfo));
    
    // Parse arguments
    if(argc > 3) {
        fprintf(stderr, "Containerfile= %s\n", argv[1]);
        fprintf(stderr, "Logfile=       %s\n", argv[2]);
        fprintf(stderr, "Mountpoint=    %s\n", argv[3]);

        // Container & log file name will be passed to fuse functions
        FsInfo->contFile= argv[1];
        FsInfo->logFile= argv[2];
                
        // Adjust arguments
        argv+= 2; argc-= 2;
    } else {
        fprintf(stderr, "Usage: %s containerfile.bin logfile mountpoint\n", argv[0]);
        return (EXIT_FAILURE);
    }
    
    fuse_stat = fuse_main(argc, argv, &myfs_oper, FsInfo);
    
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);
    
    // Cleanup
    free(FsInfo);
    
    return fuse_stat;
}


