//
//  mk.myfs.cpp
//  myfs
//
//  Created by Oliver Waldhorst on 07.09.17.
//  Copyright © 2017 Oliver Waldhorst. All rights reserved.
//

#define NAME_LENGTH 255;
#define BLOCK_SIZE 512;
#define NUM_DIR_ENTRIES 64;
#define NUM_OPEN_FILES 64;

#include "myfs.h"
#include "blockdevice.h"
#include "macros.h"
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <string.h>
#include <cstring>
#include <time.h>
#include <bitset>


#include <sstream>
#include <string>
#include <iostream>

using namespace std;

//fusermount -u ORDNER
//./mkmyfs.myfs zum starten
//make -f Makefile <- builden
/*
static int my_getattr(const char *path, struct stat *st) {
	st->st_uid = getuid(); // The owner of the file/directory is the user who mounted the filesystem
	st->st_gid = getgid(); // The group of the file/directory is the same as the group of the user who mounted the filesystem
	st->st_atime = time( NULL); // The last "a"ccess of the file/directory is right now
	st->st_mtime = time( NULL); // The last "m"odification of the file/directory is right now

	if (strcmp(path, "/") == 0) {
		st->st_mode = S_IFDIR | 0444;
		st->st_nlink = 2;
	} else {
		st->st_mode = S_IFREG | 0644;
		st->st_nlink = 1;
		st->st_size = 1024;
	}

	return 0;
}

static int my_readdir(const char *path, void *buffer, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi) {
	printf("--> Getting The List of Files of %s\n", path);

	filler(buffer, ".", NULL, 0); // Current Directory
	filler(buffer, "..", NULL, 0); // Parent Directory

	if (strcmp(path, "/") == 0) {
		filler(buffer, "Datei 1", NULL, 0);
		filler(buffer, "Datei 2", NULL, 0);
	}

	return 0;
}

static int my_read(const char *path, char *buf, size_t size, off_t offset, fuse_file_info *info) {
	int fd;
	int res;

	fd = open(path, O_RDONLY);
	if (fd == -1)
		return -errno;

	res = pread(fd, buf, size, offset);
	if (res == -1)
		res = -errno;

	close(fd);
	return res;
}

static int my_link(const char *from, const char *to) {
	int res;

	res = link(from, to);
	if (res == -1)
		return -errno;

	return 0;
}

static struct fuse_operations operations = {
    .getattr = my_getattr,
    .readlink = NULL,
    .getdir = NULL,
    .mknod = NULL,
    .mkdir = NULL,
    .unlink = NULL,
    .rmdir = NULL,
    .symlink = NULL,
    .rename = NULL,
    .link = my_link,
    .chmod = NULL,
    .chown = NULL,
    .truncate = NULL,
    .utime = NULL,
    .open = NULL,
    .read = my_read,
    .write = NULL,
    .statfs = NULL,
    .flush = NULL,
    .release = NULL,
    .fsync = NULL,
    .setxattr = NULL,
    .getxattr = NULL,
    .listxattr = NULL,
    .removexattr = NULL,
    .opendir = NULL,
    .readdir = my_readdir,
};*/

BlockDevice* bd;
uint32_t const FAT_ENDE = 3;
uint32_t const FAT_START = 1;
uint32_t const NODE_START = 4;
uint32_t const NODE_ENDE = 9;
uint32_t const DATA_START = 10;
uint32_t const MAX_UINT = -1;

struct Inode {
	char* fileName; //max 255bytes (FileName + 24bytes) (2 Für die Länge)
	uint32_t size;	//4byte
	short gid;		//2byte
	short uid;		//2byte
	short mode;		//2byte
	uint32_t atim;	//4byte
	uint32_t mtim;	//4byte
	uint32_t ctim;	//4byte
};

void getPath(char *arg, char* path);
uint32_t findNextFreeBlock();
void setFATBlockPointer(uint32_t blockPointer, uint32_t nextPointer);
uint32_t readFAT(uint32_t blockPointer);
void fillBlocks(uint32_t from, uint32_t to);
void copyFile(BlockDevice *bd, char* path);
void createInode(char* path, uint32_t blockPointer);
void writeInode(char* data);
Inode readNode(uint32_t nodePointer);
void appendString(string *s, uint32_t append);
//TODO: readDataBlock(uint32_t blockPointer)
//TODO: readFATPointer(uint32_t xyz)


void getPath(char *arg, char* path){
	char cwd[1024];
	if (getcwd(cwd, sizeof(cwd)) == NULL)
	   return;

	string path1 = cwd;
	string path2 = arg;
	string path3 = path1 + "/" + path2;
	strncpy(path, path3.c_str(), 1024);
}

uint32_t readFAT(uint32_t blockPointer){
	blockPointer -= DATA_START;
	int offsetBlockNR = blockPointer / 128;
	int offsetBlockPos = blockPointer % 128;

	char read[512];
	bd->read(FAT_START + offsetBlockNR, read); //FAT Start + Offset des Blocks

	uint32_t ergebnis = 0;
	uint32_t k = 2147483648;
	for (int i = (32 * offsetBlockPos); i < (32 * (offsetBlockPos + 1)); i++){
		ergebnis += ((unsigned short)read[i] / 65535) * k;
		k/=2;
	}

	return ergebnis;
}

void fillBlocks(uint32_t from, uint32_t to){
	char fill[512];
	for (int i = 0; i < 512; i++)
		fill[i] = 0; //255 = 1

	for (uint32_t i = from; i < to; i++)
		bd->write(i, fill);
}

void copyFile(BlockDevice *bd, char* path){
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
	char data[512];
	if (strlen(string) > 512) { //wir benötigen mehr als nur ein Block
		while((unsigned)(position * 512) < strlen(string)){
			for (int i = 0; i < 512; i++)	//füllen der Daten zum schreiben
				data[i] = string[(position * 512) + i];

			bd->write(blockPointer, data);
			setFATBlockPointer(blockPointer, MAX_UINT); //block auf belegt setzen
			oldPointer = blockPointer;

			blockPointer = findNextFreeBlock();	//neuen Block holen
			setFATBlockPointer(oldPointer, blockPointer);	//verweiß setzen
			position++;
		}
	} else {	//izi. nur ein Block
		bd->write(blockPointer, string);
		setFATBlockPointer(blockPointer, MAX_UINT);
	}
}

void appendString(string *s, uint32_t append){
	char data[4];
	memcpy(data, &append, sizeof(append));
	s->append(data, 4);
}

void appendString(string *s, short append){
	char data[2];
	memcpy(data, &append, sizeof(append));
	s->append(data, 2);
}

void createInode(char* path, uint32_t blockPointer) {
	Inode node;
	char* chars_array = strtok(path, "/");

	struct stat meta;
	stat(path, &meta);

	node.fileName = chars_array;
	node.size = meta.st_size;
	node.gid = meta.st_gid;
	node.uid = meta.st_uid;
	node.mode = meta.st_mode;

	node.atim = meta.st_atim.tv_sec;
	node.mtim = meta.st_mtim.tv_sec;
	node.ctim = meta.st_ctim.tv_sec;

	string s;
	s.append(node.fileName);
    appendString(&s, node.size);
    appendString(&s, node.gid);
    appendString(&s, node.uid);
    appendString(&s, node.mode);
    appendString(&s, node.atim);
    appendString(&s, node.mtim);
    appendString(&s, node.ctim);

    char data[512];
    strcpy(data, s.c_str());

    for (int i = 0; i < s.size(); i++)
    	cout << (unsigned)data[i] << endl;
    cout << endl;

    writeInode(data);
}
/*
 * Er fängt direkt an data zu lesen, es ist vermutlich noch ein Fehler in dem Schreiben der passenden Daten.
 * Zumindest werden sie (meiner Meinung nach) bisher richtig gebildet. */
void writeInode(char* data) {
	char read[512];

	for (uint32_t i = NODE_START; i < NODE_ENDE; i++){
		bd->read(i, read);
		if (read[0] == 0) {	//Leerer Block
			cout << "Inode block der frei ist: " << i << endl;
			bd->write(i, data);
			return;
		}
	}
}

Inode readNode(uint32_t nodePointer){
	Inode node;
	int currentPointer = 0;
	char read[512];

	for (int i = NODE_START; i < NODE_ENDE; i++){
		if (nodePointer == currentPointer) {	//Richtige Node
			bd->read(i, read);
			char name[255];
			for (short k = 0; k < 256; k++){ //Name = MAX 255
				name[k] = read[k];
			}
			strcpy(node.fileName, name);
		}
	}

	return node;
}

void setFATBlockPointer(uint32_t blockPointer, uint32_t nextPointer){ //0x00000000 = FREI | 0xFFFFFFFF = BELEGT
	blockPointer -= DATA_START;
	int offsetBlockNR = blockPointer / 128;
	int offsetBlockPos = blockPointer % 128;

	char read[512];
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
	char read[512];
	for(uint32_t i = FAT_START; i < FAT_ENDE; i++){
		bd->read(i, read);
		for (int x = 0; x < 128; x++){	//Max. ints in einem Block
			ergebnis = 0;
			for(int m = (4 * x); m < (4 * (x+1)); m++){	//Bauen der Zahl
				ergebnis <<= 8;
				ergebnis |= read[m];
			}
			if (ergebnis == 0)//4294967295)	//Wir einigten uns darauf das 0xFFFFFFFF = leer bedeutet
				return ((i - FAT_START) * 128) + x + DATA_START; //Alles damit wir in die Datenblöcke kommen
		}
	}
	cout << "wops, something went wrong!" << endl;
	return 0; //RETURN ERROR: FileSystem FULL!
}

int main(int argc, char *argv[]) {

	printf("\n\n");
	if (argc < 3)
		printf("using: mkfs.myfs containerdatei [input-datei ...]\n");

	char path[1024];
	getPath(argv[1], path);

	bd = new BlockDevice(512);
	bd->create(path);
	fillBlocks(0, 16);

	for (int i = 2; i < 3; i++)
		copyFile(bd, argv[i]);

	char output[1];
	for (int i = 6; i < 12; i++) {
		bd->read(i, output);
		//cout << output << endl;
	}

	Inode node = readNode(0);
	cout << node.fileName << endl;

	bd->close();
	return 0;
}
