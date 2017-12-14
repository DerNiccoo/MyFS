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
MyFSMgr* MyFSMgr::instance() {
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
}
MyFSMgr::~MyFSMgr() {
}


/**
 * Determine the full path of a given file which must be inside the current working directory.
 * getcwd depends on unistd.h
 * strncpy depends on cstring
 *
 * @param arg The name of a file inside the current working directory.
 * @param path out The absolute path of the file.
 */
void MyFSMgr::getAbsPath(char* arg, char* path) {
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
void MyFSMgr::fillBlocks(uint32_t startBlockIndex, uint32_t endBlockIndex) {
    char rawBlock[BLOCK_SIZE];
    for (int i = 0; i < BLOCK_SIZE; i++)
        rawBlock[i] = 0; // 255 = 1

    for (uint32_t i = startBlockIndex; i < endBlockIndex; i++)
        _blockDevice->write(i, rawBlock);
}

/**
 * Write the SuperBlock into our FileSystem. The SuperBlock is always
 * the first block in a FileSystem and contains some information about
 * a file system.
 */
void MyFSMgr::writeSuperBlock() {
    char copy[BLOCK_SIZE];
    Superblock* sb = (Superblock*) copy;
    LOGF("\n| SUPER BLOCK | %u FAT %u | %u INODES %u | %u ROOT BLOCK | %u DATA %u |\n\n",
         FAT_START, FAT_ENDE, NODE_START, NODE_ENDE, ROOT_BLOCK, DATA_START, SYSTEM_SIZE);

    sb->size = SYSTEM_SIZE;
    sb->pointerData = DATA_START;
    sb->pointerFat = FAT_START;
    sb->pointerNode = NODE_START;
    sb->fileCount = 0;

    _blockDevice->write(0, (char*) sb);
}

/**
 * Change the stored file count inside the super block.
 *
 * @param difference The signed amount to inc-/decrement.
 */
void MyFSMgr::changeSBFileCount(int difference) {
    char read[BLOCK_SIZE];
    _blockDevice->read(0, read);

    Superblock* sb = (Superblock*) read;
    if (difference > 0) {
        // Addition
        sb->fileCount += difference;
    } else if (difference != 0 && sb->fileCount >= (uint32_t) abs(difference)) {
        // Subtraction
        sb->fileCount += difference;
    } else {
        LOGF("Invalid difference value %d!", difference);
    }
    _blockDevice->write(0, (char*) sb);
}

/**
 * Imports a file to our FileSystem. Checks if the file already exists,
 * if not creates an Inode and write the data into the DataBlocks. If
 * needed links multiple blocks together in the FAT.
 *
 * @param path The path to the file that should be imported.
 * @return  0 On success.
 *          -1 If the file already exists.
 */
int MyFSMgr::importFile(char* path) {
    if (fileExists(path))
        return -1;

    FILE* f = fopen(path, "rb"); // Read binary file
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* fileContent = (char*) malloc(fsize + 1);
    fread(fileContent, fsize, 1, f);
    fclose(f);

    int position = 0;
    uint32_t blockPointer = findNextFreeBlock();
    //if (blockPointer == 0) // TODO Chris: Handle errors like FileSystem full
    //    return -1;

    uint32_t oldPointer;
    createInode(path, blockPointer);
    char data[BLOCK_SIZE];
    if (strlen(fileContent) > BLOCK_SIZE) {         // Wir benötigen mehr als nur ein Block
        while((unsigned)(position * BLOCK_SIZE) < strlen(fileContent)) {
            for (int i = 0; i < BLOCK_SIZE; i++)    // Füllen der Daten zum schreiben
                data[i] = fileContent[(position * BLOCK_SIZE) + i];

            _blockDevice->write(blockPointer, data);
            setFATBlockPointer(blockPointer, MAX_UINT);   // Block auf belegt setzen
            oldPointer = blockPointer;

            blockPointer = findNextFreeBlock();           // Neuen Block holen
            setFATBlockPointer(oldPointer, blockPointer); // Verweiß setzen
            position++;
        }
    } else { // Izi. nur ein Block
        _blockDevice->write(blockPointer, fileContent);
        setFATBlockPointer(blockPointer, MAX_UINT);
    }
    changeSBFileCount(1);
    return 0;
}

/**
 * Search for a free DataBlock. Loops through the FAT and checks every
 * block if its empty (=0)
 *
 * @return  A pointer to a free DataBlock
 *          0 If the FileSystem is full
 */
uint32_t MyFSMgr::findNextFreeBlock() {
    uint32_t result;

    char read[BLOCK_SIZE];
    for(uint32_t i = FAT_START; i <= FAT_ENDE; i++) {
        _blockDevice->read(i, read);

        for (int x = 0; x < 128; x++) { // Max. ints in einem Block
            result = 0;
            for(int m = (4 * x); m < (4 * (x + 1)); m++) { // Bauen der Zahl
                result <<= 8;
                result |= read[m];
            }

            if (result == 0) // wir einigten uns darauf das 0 = frei bedeutet
                return ((i - FAT_START) * 128) + x + DATA_START; // Alles damit wir in die Datenblöcke kommen
        }
    }

    LOG("Woops, something went wrong!");
    return 0; // TODO RETURN ERROR: FileSystem FULL!
}

/**
 * Create a Inode of a given file. A Inode contains meta
 * data about the file.
 *
 * @param path The path to the file.
 * @param blockPointer The pointer to the first DataBlock.
 */
void MyFSMgr::createInode(char* path, uint32_t blockPointer) {
    char copy[BLOCK_SIZE]; // Max. size - is ALWAYS BLOCK_SIZE (512).
    Inode* node = (Inode*) copy;
    char* pathSegments = strtok(path, "/");

    struct stat meta;   //Holt sich die meta Daten von einer file.
    stat(path, &meta);

    strcpy(node->fileName, pathSegments);
    node->size = meta.st_size;  //Attribut aus den meta informationen
    node->gid = meta.st_gid;
    node->uid = meta.st_uid;
    node->mode = meta.st_mode;

    node->atim = meta.st_atim.tv_sec;
    node->mtim = meta.st_mtim.tv_sec;
    node->ctim = meta.st_ctim.tv_sec;

    node->pointer = blockPointer;

    writeInode(node);
}

/**
 * [BUGGED]Creates an empty Inode for a new file.
 *
 * @param path The name of the file.
 */
void MyFSMgr::createNewInode(const char* path){	//Leere Datei, hat sie einen BlockPointer?
    char copy[BLOCK_SIZE];
    Inode* node = (Inode*) copy;

    strcpy(node->fileName, path);
    node->size = 0;
    node->gid = getgid();
    node->uid = getuid();
    node->mode = S_IFDIR | 0755;
    node->atim = time( NULL );
    node->mtim = time( NULL );
    node->ctim = time( NULL );

    writeInode(node);
}

/**
 * Write the provided Inode to the first empty block.
 *
 * @param node The Inode to be written.
 */
void MyFSMgr::writeInode(Inode* node) {
    char read[BLOCK_SIZE];

    for (uint32_t pointer = NODE_START; pointer <= NODE_ENDE; pointer++) {
        _blockDevice->read(pointer, read);

        if (read[0] == 0) { // Block is empty
            _blockDevice->write(pointer, (char*) node);
            writeRootPointer(pointer);
            return;
        }
    }
}

/**
 * Write a pointer to the given place in the FAT. If one block isn't enough
 * we need more blocks to address all the data for a single file. Each block
 * has a pointer to the next DataBlock. This way we have a 'chain' with pointers
 * to the DataBlocks.
 *
 * 0x00000000 = FREI | 0xFFFFFFFF = BELEGT
 *
 * @param blockPointer  The current position which points to the next position.
 * @param nextPointer   The next position that points to more data.
 */
void MyFSMgr::setFATBlockPointer(uint32_t blockPointer, uint32_t nextPointer) {
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
 * Write a pointer to the Rootblock in an empty place.
 *
 * @param newPointer    The pointer that will be filled in the Rootblock.
 */
void MyFSMgr::writeRootPointer(uint32_t newPointer) {
    char read[BLOCK_SIZE];

    _blockDevice->read(ROOT_BLOCK, read);
    for (int i = 0; i < BLOCK_SIZE; i += 4) {
        uint32_t pointer = read[i] << 24 | read[i + 1] << 16 | read[i + 2] << 8 | read[i + 3];
        //shifting für 4 Byte zahl.
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
bool MyFSMgr::fileExists(char* path) {
    char copy[BLOCK_SIZE];
    Inode* node = (Inode*)copy;
    uint32_t position = MAX_UINT;
    char* pathSegments = strtok(path, "/");
    char fileName[NAME_LENGTH];

    strcpy(fileName, pathSegments);

    if (rootPointerCount() == 0) // The first file can't be a duplicate
        return false;

    while((position = readNextRootPointer(position)) != 0){

        _blockDevice->read(position, (char*)node);
        LOGF("%s : %s\n", fileName, node->fileName);
        if (strcmp(fileName, node->fileName) == 0)
            return true;
    }

    return false;
}

/**
 * Return the sum of all the pointers to an Inode inside the Rootblock.
 *
 * @return The sum of all pointers. 0 if none.
 */
int MyFSMgr::rootPointerCount() {
    char read[BLOCK_SIZE];
    int sum = 0;

    _blockDevice->read(ROOT_BLOCK, read);

    for (int i = 0; i < BLOCK_SIZE; i += 4) {
        uint32_t pointer = read[i] << 24 | read[i + 1] << 16 | read[i + 2] << 8 | read[i + 3];
        if (pointer != 0)   //Bit shifting zum zusammensetzen der 4Byte langen int Zahl.
            sum++;
    }
    return sum;
}

/**
 * Return the following pointer of an Inode from the Rootblock.
 *
 * @param   oldPointer The pointer of the old Inode. If set to -1 return the first Inode pointer.
 *
 * @return  0 if it was the last Pointer.
 *          else the pointer to the next Inode.
 */
uint32_t MyFSMgr::readNextRootPointer(uint32_t oldPointer) {
    char read[BLOCK_SIZE];

    _blockDevice->read(ROOT_BLOCK, read);
    for (int i = 0; i < BLOCK_SIZE; i += 4) {
        uint32_t pointer = read[i] << 24 | read[i + 1] << 16 | read[i+2] << 8 | read[i + 3];
        //Bit shifting zum zusammensetzen der 4Byte langen int Zahl.
        if (pointer != 0 && oldPointer == MAX_UINT)
            return pointer;

        if (pointer == oldPointer)
            oldPointer = MAX_UINT;
    }

    return 0;
}

/**
 * Read FAT entry of given pointer.
 *
 * @param blockPointer Pointer which points to a entry inside the FAT.
 * @return The linked value inside the FAT of the provided blockPointer.
 */
uint32_t MyFSMgr::readFAT(uint32_t blockPointer){
    blockPointer -= DATA_START;
    int offsetBlockNR = blockPointer / 128; //128 Pointer pro Block da ganzzahl Ergebnis = Block
    int offsetBlockPos = blockPointer % 128;//Position in dem Block

    char read[512];
    _blockDevice->read(FAT_START + offsetBlockNR, read); // FAT Start + Offset des Blocks


    uint32_t pointer = read[4 * offsetBlockPos+3] << 24 | read[4 * offsetBlockPos+2] << 16 | read[4 * offsetBlockPos+1] << 8 | read[4 * offsetBlockPos+0];

    return pointer;
}

/**
 * Read the given node of the Pointer.
 * UNUSED.
 *
 * @param nodePointer The Pointer of the node to read.
 * @return The node of the given Pointer.
 */
Inode* MyFSMgr::readNode(uint32_t nodePointer){
    char copy[BLOCK_SIZE];
    Inode* node = (Inode*)copy;
    uint32_t currentPointer = 0;

    for (unsigned int i = NODE_START; i < NODE_ENDE; i++){
        if (nodePointer == currentPointer) { // Richtige Node
            _blockDevice->read(i, (char*) &node);
            return node;
        }
        currentPointer++;
    }

    return node;
}

/**
 * Remove the file from MyFS and all dependencies.
 *
 * @param nodePointer The Pointer of the file to remove.
 */
void MyFSMgr::removeFile(uint32_t nodePointer) {
    char copy[512];
    Inode* node = (Inode*)copy;
    _blockDevice->read(nodePointer, (char*)node); // Node die gelöscht werden soll

    uint32_t pointer = node->pointer;
    fillBlocks(pointer, pointer + 1);

    while ((pointer = readFAT(pointer)) != UINT32_MAX) { // Die Datei war größer als 1 Block, daher einträge in der FAT die gelöscht werden müssen
        fillBlocks(pointer, pointer + 1);
        removeFatPointer(pointer);
    }

    removeRootPointer(nodePointer);           // löschen im RootBlock
    fillBlocks(nodePointer, nodePointer + 1); // löschen der Node
}

/**
 * Remove the FAT pointer to a provided file.
 *
 * @param delPointer The pointer that should be deleted.
 */
void MyFSMgr::removeFatPointer(uint32_t delPointer) {
    char read[512];

    for (uint32_t i = FAT_START; i < FAT_ENDE; i++){
        _blockDevice->read(i, read);
        for (int j = 0; j < 512; j+=4) {
            uint32_t pointer = read[j] << 24 | read[j+1] << 16 | read[j+2] << 8 | read[j+3];

            if (pointer == delPointer) {
                for (int k = i; k < (j + 4); k++) // den Pointer mit dem Wert nullen.
                    read[k] = 0;

                _blockDevice->write(i, read); // neuen block schreiben
                return;
            }
        }
    }

}

/**
 * Remove the Root pointer to a provided file.
 *
 * @param delPointer The pointer that should be deleted.
 */
void MyFSMgr::removeRootPointer(uint32_t delPointer) {
    char read[512];

    _blockDevice->read(ROOT_BLOCK, read);
    for (int i = 0; i < 512; i+=4) {
        uint32_t pointer = read[i] << 24 | read[i+1] << 16 | read[i+2] << 8 | read[i+3];

        if (pointer == delPointer) {
            for (int k = i; k < (i + 4); k++) // den Pointer mit dem Wert nullen.
                read[k] = 0;
            _blockDevice->write(ROOT_BLOCK, read); // schreiben damit der loop beendet werden kann
            return;
        }
    }
}

/**
 * Moves the buffer that the first Bytes are in the offset block range.
 *
 * @param db The DataBuffer with the buffer that should be moved.
 * @param off The offset for the Bytes that will be readed.
 */
int MyFSMgr::moveBuffer(DataBuffer* db, int off) {
    char read[BLOCK_SIZE];
    LOGF("%u\n", db->dataPointer);
    uint32_t blockOffset = off / BLOCK_SIZE; //The Block that should the Buffer contains
    if (blockOffset == db->blockNumber)
        return 0;

    while (blockOffset != db->blockNumber){
        db->dataPointer = MyFSMgr::instance()->readFAT(db->dataPointer);
        if (db->dataPointer == MAX_UINT)
            return -1;
        LOGF("%u\n", db->blockNumber);
        db->blockNumber++;
    }
    MyFSMgr::BDInstance()->read(db->dataPointer, read);
    memcpy(&db->data, read, 512);
    return 0;
}
