/*
 * MyFSMgr.cpp
 *
 *  Created on: Dec 3, 2017
 *      Author: FireEmerald
 */

#include "MyFSMgr.h"

using namespace std;

MyFSMgr* MyFSMgr::_instance = NULL;
BlockDevice* MyFSMgr::_blockDevice = NULL;

/**
 * Access the MyFS manager instance.
 *
 * @return The current MyFS manager instance.
 */
MyFSMgr* MyFSMgr::Instance() {
    if(_instance == NULL) {
        _instance = new MyFSMgr();
    }
    return _instance;
}

/**
 * Access the BlockDevice instance.
 *
 * @return The current BlockDevice instance.
 */
BlockDevice* MyFSMgr::BDInstance() {
    if(_blockDevice == NULL) {
        _blockDevice = new BlockDevice(BLOCK_SIZE);
    }
    return _blockDevice;
}


MyFSMgr::MyFSMgr() {
    Logger::GetLogger()->Log("MyFSMgr constructor called");
}
MyFSMgr::~MyFSMgr() {
    Logger::GetLogger()->Log("MyFSMgr destructor called");
}


/**
 * Determine the full path of a given file which must be inside the current working directory.
 * getcwd depends on unistd.h
 * strncpy depends on cstring
 *
 * @param arg The name of a file inside the current working directory.
 * @param path out The absolute path of the file.
 */
void MyFSMgr::GetAbsPath(char* arg, char* path) {
    char currentWorkingDirectory[1024];

    // Get working directory and check if it couldn't be determined or SIZE was too small
    if (getcwd(currentWorkingDirectory, sizeof(currentWorkingDirectory)) == NULL)
       return;

    string absolutePath = (string)currentWorkingDirectory + "/" + (string)arg;
    strncpy(path, absolutePath.c_str(), 1024);
}

/**
 * Fill the specified blocks of the BlockDevice with zeros.
 *
 * @param startBlockIndex Index of the first block to fill.
 * @param endBlockIndex Index of the last block to fill.
 */
void MyFSMgr::FillBlocks(uint32_t startBlockIndex, uint32_t endBlockIndex) {
    char rawBlock[BLOCK_SIZE];
    for (int i = 0; i < BLOCK_SIZE; i++)
        rawBlock[i] = 0; // 255 = 1

    for (uint32_t i = startBlockIndex; i < endBlockIndex; i++)
        _blockDevice->write(i, rawBlock);
}

/**
 * TODO Chris: Fill
 */
void MyFSMgr::WriteSuperBlock() {
    char copy[BLOCK_SIZE];
    Superblock* sb = (Superblock*) copy;

    sb->Size = SIZE;
    sb->PointerData = DATA_START;
    sb->PointerFat = FAT_START;
    sb->PointerNode = NODE_START;

    _blockDevice->write(0, (char*) sb);
}


/**
 * TODO Chris: Fill
 *
 * @param path
 * @return
 */
int MyFSMgr::ImportFile(char* path) {
    if (FileExists(path))
        return -1;

    FILE* f = fopen(path, "rb"); // Read binary file
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* fileContent = (char*) malloc(fsize + 1);
    fread(fileContent, fsize, 1, f);
    fclose(f);

    int position = 0;
    uint32_t blockPointer = FindNextFreeBlock();
    //if (blockPointer == 0) // TODO Chris: Handle errors like FileSystem full
    //    return -1;

    uint32_t oldPointer;
    CreateInode(path, blockPointer);
    char data[BLOCK_SIZE];
    if (strlen(fileContent) > BLOCK_SIZE) {         // Wir benötigen mehr als nur ein Block
        while((unsigned)(position * BLOCK_SIZE) < strlen(fileContent)) {
            for (int i = 0; i < BLOCK_SIZE; i++)    // Füllen der Daten zum schreiben
                data[i] = fileContent[(position * BLOCK_SIZE) + i];

            _blockDevice->write(blockPointer, data);
            SetFATBlockPointer(blockPointer, MAX_UINT);   // Block auf belegt setzen
            oldPointer = blockPointer;

            blockPointer = FindNextFreeBlock();           // Neuen Block holen
            SetFATBlockPointer(oldPointer, blockPointer); // Verweiß setzen
            position++;
        }
    } else { // Izi. nur ein Block
        _blockDevice->write(blockPointer, fileContent);
        SetFATBlockPointer(blockPointer, MAX_UINT);
    }
    return 0;
}

/**
 * TODO Chris: Fill
 *
 * @return
 */
uint32_t MyFSMgr::FindNextFreeBlock() {
    uint32_t result;

    char read[BLOCK_SIZE];
    for(uint32_t i = FAT_START; i < FAT_ENDE; i++) {
        _blockDevice->read(i, read);

        for (int x = 0; x < 128; x++) { // Max. ints in einem Block
            result = 0;
            for(int m = (4 * x); m < (4 * (x + 1)); m++) { // Bauen der Zahl
                result <<= 8;
                result |= read[m];
            }

            if (result == 0) // 4294967295, wir einigten uns darauf das 0xFFFFFFFF = leer bedeutet
                return ((i - FAT_START) * 128) + x + DATA_START; // Alles damit wir in die Datenblöcke kommen
        }
    }
    cout << "Woops, something went wrong!" << endl;
    return 0; // TODO RETURN ERROR: FileSystem FULL!
}

/**
 * TODO Chris: Fill
 *
 * @param path
 * @param blockPointer
 */
void MyFSMgr::CreateInode(char* path, uint32_t blockPointer) { // TODO Chris: blockPointer is not implemented.
    char copy[BLOCK_SIZE];                  // Max. size - is ALWAYS BLOCK_SIZE (512).
    Inode* node = (Inode*) copy;
    char* pathSegments = strtok(path, "/");

    struct stat meta;
    stat(path, &meta);

    strcpy(node->fileName, pathSegments);
    node->size = meta.st_size;
    node->gid = meta.st_gid;
    node->uid = meta.st_uid;
    node->mode = meta.st_mode;

    node->atim = meta.st_atim.tv_sec;
    node->mtim = meta.st_mtim.tv_sec;
    node->ctim = meta.st_ctim.tv_sec;

    WriteInode(node);
}

/**
 * Write the provided Inode to the first empty block.
 *
 * @param node The Inode to be written.
 */
void MyFSMgr::WriteInode(Inode* node) {
    char read[BLOCK_SIZE];

    for (uint32_t pointer = NODE_START; pointer < NODE_ENDE; pointer++) {
        _blockDevice->read(pointer, read);

        if (read[0] == 0) { // Block is empty
            _blockDevice->write(pointer, (char*) node);
            WriteRootPointer(pointer);
            return;
        }
    }
}

/**
 * TODO Chris: Fill
 *
 * 0x00000000 = FREI | 0xFFFFFFFF = BELEGT
 *
 * @param blockPointer
 * @param nextPointer
 */
void MyFSMgr::SetFATBlockPointer(uint32_t blockPointer, uint32_t nextPointer) {
    blockPointer -= DATA_START;
    int offsetBlockNR = blockPointer / 128;
    int offsetBlockPos = blockPointer % 128;

    char read[BLOCK_SIZE];
    _blockDevice->read(FAT_START + offsetBlockNR, read);

    char data[4];
    if (nextPointer != 0) {
        memcpy(data, &nextPointer,sizeof(uint32_t));
    } else {
        for (int i = 0; i < 4; i++)
            data[i] = 0;
    }

    int k = 0;
    for (int i = (4 * offsetBlockPos); i < (4 * (offsetBlockPos + 1)); i++) {
        read[i] = data[k];
        k++;
    }

    _blockDevice->write(FAT_START + offsetBlockNR, read);
}

/**
 * TODO Chris: Fill
 *
 * @param newPointer
 */
void MyFSMgr::WriteRootPointer(uint32_t newPointer) {
    char read[BLOCK_SIZE];

    _blockDevice->read(ROOT_BLOCK, read);
    for (int i = 0; i < BLOCK_SIZE; i += 4) {
        uint32_t pointer = read[i] << 24 | read[i + 1] << 16 | read[i + 2] << 8 | read[i + 3];

        if (pointer == 0) {
            char data[4];
            memcpy(data, &newPointer, 4);
            for (int k = 0; k < 4; k++) // Damit die zahlen im normalen Style sind 0011 = 3 und nicht 1100 = 3
                read[i + 3 - k] = data[k];

            _blockDevice->write(ROOT_BLOCK, read);
            return;
        }
    }
}

/**
 * Check if the provided file exists inside the BlockDevice.
 *
 * @param path The path of the file to check.
 * @return True if the file does exist, otherwise false.
 */
bool MyFSMgr::FileExists(char* path) {
    Inode node;
    uint32_t position = MAX_UINT;
    char* pathSegments = strtok(path, "/");
    char fileName[NAME_LENGTH];

    strcpy(fileName, pathSegments);

    cout << fileName << endl;

    if (RootPointerCount() == 0) // The first file can't be a duplicate
        return false;

    while((position = ReadNextRootPointer(position)) != 0){

        _blockDevice->read(position, (char*) &node);

        cout << fileName << " : " << node.fileName << endl;
        if (fileName == node.fileName)
            return true;
    }

    return false;
}

/**
 * TODO Chris: Fill
 *
 * @return
 */
int MyFSMgr::RootPointerCount() {
    char read[BLOCK_SIZE];
    int sum = 0;

    _blockDevice->read(ROOT_BLOCK, read);

    for (int i = 0; i < BLOCK_SIZE; i += 4) {
        uint32_t pointer = read[i] << 24 | read[i + 1] << 16 | read[i + 2] << 8 | read[i + 3];
        if (pointer != 0)
            sum++;
    }
    return sum;
}

/**
 * TODO Chris: Description...
 *
 * @param position -1 = Erster Treffer
 *                 PointerPosition = Es wird die NÄCHSTE zurückgegeben (damit lücken übersprungen werden, wenn man z.B. etwas löscht)
 * @return 0 = war der letzte Pointer sonst gibt es immer den nächsten Pointer
 */
uint32_t MyFSMgr::ReadNextRootPointer(uint32_t position) {
    char read[BLOCK_SIZE];

    _blockDevice->read(ROOT_BLOCK, read);
    for (int i = 0; i < BLOCK_SIZE; i += 4) {
        uint32_t pointer = read[i] << 24 | read[i + 1] << 16 | read[i+2] << 8 | read[i + 3];

        if (pointer != 0 && position == MAX_UINT)
            return pointer;

        if (pointer == position)
            position = MAX_UINT;
    }

    return 0;
}
