//-fs.c: 과제1에서 구현한 코드를 이 파일에 넣고, 빈 함수를 구현하면 됩니다.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "disk.h"
#include "fs.h"
#include <math.h>
#include <stdbool.h>

const int NUM_OF_BLOCKS_IN_INDIRECTBLOCK = BLOCK_SIZE / sizeof(int);

FileDescTable *pFileDescTable = NULL;
#define START_INDEX_DATA_REGION 19
typedef enum _UPDATE_FLAG {
    ADD_DIR,
    ALOCATE_BLOCK,
    ALOCATE_INODE,
    FREE_BLOCK,
    FREE_INODE
} UPDATE_FLAG;

int initNewFile(int newInodeNum) {

    int i;
    for (i = 0; i < MAX_FD_ENTRY_LEN; i++) {
        if (pFileDescTable->file[i].bUsed == 0) {
            pFileDescTable->file[i].bUsed = 1;
            pFileDescTable->file[i].inodeNum = newInodeNum;
            pFileDescTable->file[i].fileOffset = 0;
            return i;
        }
    }
    return -1;
}

int addNewDirEntry(int index, Inode *parentInode) {//openfile, makedir

    int newBlockNum = GetFreeBlockNum();
    updateFileSysInfo(ALOCATE_BLOCK);
    SetBlockBitmap(newBlockNum);

    //init new Entry Block
    DirEntry newDirEntry[NUM_OF_DIRENT_PER_BLOCK];
    memset(newDirEntry, 0, BLOCK_SIZE);

    //link with parent Block
    parentInode->dirBlockPtr[index] = newBlockNum; //ex) 19

    DevWriteBlock(newBlockNum, newDirEntry);

    return newBlockNum;
}

void updateFileSysInfo(UPDATE_FLAG flag) {
    if (pFileSysInfo == NULL) {
        pFileSysInfo = (FileSysInfo *) malloc(sizeof(FileSysInfo));
        DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
    }
    switch (flag) {
        case ADD_DIR:
            pFileSysInfo->numAllocBlocks++;
            pFileSysInfo->numFreeBlocks--;
            pFileSysInfo->numAllocInodes++;
            break;
        case ALOCATE_BLOCK:
            pFileSysInfo->numAllocBlocks++;
            pFileSysInfo->numFreeBlocks--;
            break;
        case ALOCATE_INODE:
            pFileSysInfo->numAllocInodes++;
            break;
        case FREE_BLOCK:
            pFileSysInfo->numAllocBlocks--;
            pFileSysInfo->numFreeBlocks++;
            break;
        case FREE_INODE:
            pFileSysInfo->numAllocInodes--;
            break;
    }
    DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
}

int addNewDirBlock(char *target, int parentInodeNum, int *parentBlockNum, DirEntry *parentDE) {
//only Makedir

    for (int i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++) {
        if (strcmp(parentDE[i].name, "") == 0) {

            int newInodeNum = GetFreeInodeNum();
            updateFileSysInfo(ALOCATE_INODE);
            SetInodeBitmap(newInodeNum);
            Inode *newInode = (Inode *) malloc(BLOCK_SIZE);
            newInode->type = FILE_TYPE_DIR;
            newInode->size = 0;
            PutInode(newInodeNum, newInode);

            strcpy(parentDE[i].name, target);
            parentDE[i].inodeNum = newInodeNum;
            DevWriteBlock((int) parentBlockNum, (char *) parentDE);

            int newBlockNum = GetFreeBlockNum();
            SetBlockBitmap(newBlockNum);
            updateFileSysInfo(ALOCATE_BLOCK);
            DirEntry newDE[NUM_OF_DIRENT_PER_BLOCK];
            memset(newDE, 0, BLOCK_SIZE);
            strcpy(newDE[0].name, ".");
            newDE[0].inodeNum = newInodeNum;
            strcpy(newDE[1].name, "..");
            newDE[1].inodeNum = parentInodeNum;
            DevWriteBlock(newBlockNum, (char *) newDE);
            DevWriteBlock((int) parentBlockNum, (char *) parentDE);

            newInode->dirBlockPtr[0] = newBlockNum;
            PutInode(newInodeNum, newInode);
            free(newInode);
            return newBlockNum;
        }
    }
    return -1;
}

int goDownDir(char *target, Inode *curInode, int *curInodeNum, int *bnum) {//이거는 많이 쓸거 같다

    DirEntry de[NUM_OF_DIRENT_PER_BLOCK];
    //just check direct block
    for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {
        *bnum = curInode->dirBlockPtr[i];

        DevReadBlock(*bnum, (char *) de);
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (strcmp(de[j].name, target) == 0) {
                GetInode(de[j].inodeNum, curInode);
                *bnum = curInode->dirBlockPtr[i];
                *curInodeNum = de[j].inodeNum;
                return *curInodeNum;
            }
        }
    }
    int inbnum = curInode->indirBlockPtr;
    int indirectBlock[BLOCK_SIZE / sizeof(int)];
    DevReadBlock(inbnum, (char *) indirectBlock);
    for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
        *bnum = indirectBlock[i];
        DevReadBlock(*bnum, de);
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (strcmp(de[j].name, target) == 0) {
                GetInode(de[j].inodeNum, curInode);
                *bnum = indirectBlock[i];
                *curInodeNum = de[j].inodeNum;
                return *curInodeNum;
            }
        }
    }
    return -1;
}

int addNewFileBlock(int index, char *target, Inode *parentInode, int parentInodeNum, int *parentBlockNum,
                    DirEntry *parentDE) {

    int newBlockNum = -1;
    int curBlockNum = parentInode->dirBlockPtr[index];
    int i;
    for (i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++) {
        if (strcmp(parentDE[i].name, "") == 0) {

            //save child DE
            int newInodeNum = GetFreeInodeNum();
            updateFileSysInfo(ALOCATE_INODE);
            SetInodeBitmap(newInodeNum);
            Inode *newInode = (Inode *) malloc(BLOCK_SIZE);
            newInode->type = FILE_TYPE_FILE;
            newInode->size = 0;
            PutInode(newInodeNum, newInode);

            //save parent DE
            strcpy(parentDE[i].name, target);
            parentDE[i].inodeNum = newInodeNum;
            DevWriteBlock(parentBlockNum, parentDE);

            //            newBlockNum = GetFreeBlockNum();
            //            SetBlockBitmap(newBlockNum);
            //            updateFileSysInfo(ALOCATE_BLOCK);
            //            char* newFile = (char *)malloc(sizeof(BLOCK_SIZE));
            //            memset(newFile,0,sizeof(BLOCK_SIZE));

            //            DevWriteBlock(newBlockNum,newFile);
            //            DevWriteBlock(parentBlockNum,parentDE);

            //            newInode->dirBlockPtr[0]=newBlockNum;
            //            PutInode(newInodeNum,newInode);
            free(newInode);
            //            free(newFile);
            return newInodeNum;
        }
    }
    return -1;
}

int addFileDir(char *target, int parentInodeNum) {
    int i, j;
    int bnum;
    int newBlockNum;
    Inode *parentInode = (Inode *) malloc(BLOCK_SIZE);
    GetInode(parentInodeNum, parentInode);
    DirEntry de[NUM_OF_DIRENT_PER_BLOCK];

//    if (isExistInDir(target, parentInode) == 1)// //필요 없을거 같
//    { //there is same name file
//        free(parentInode);
//        return -1;
//    }

    //travel direct pointers
    for (i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {

        bnum = parentInode->dirBlockPtr[i];
        if (bnum == 0) { //add new direct pointer and new entry
            newBlockNum = addNewDirEntry(i, parentInode);
            parentInode->dirBlockPtr[i] = newBlockNum;
            parentInode->size += BLOCK_SIZE;
            bnum = parentInode->dirBlockPtr[i];
            PutInode(parentInodeNum, parentInode);
        }
        DevReadBlock(bnum, de);

        //add new dir
        for (j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) { //4
            if (strcmp(de[j].name, "") == 0) {
                int newInodeNum = addNewFileBlock(i, target, parentInode, parentInodeNum, bnum, de);
                free(parentInode);
                return newInodeNum;
            }
        }
    }

    //--------indirect------

    //travel indirect block
    int indirectBlockNum = parentInode->indirBlockPtr;
    if (indirectBlockNum == 0) { //does't allocate indirect Block yet
        //add new indirect Block
        newBlockNum = GetFreeBlockNum();
        updateFileSysInfo(ALOCATE_BLOCK);
        SetBlockBitmap(newBlockNum);
        indirectBlockNum = newBlockNum;
        bnum = newBlockNum;
        int indirectBlock[NUM_OF_BLOCKS_IN_INDIRECTBLOCK];
        memset(indirectBlock, 0, BLOCK_SIZE);
        parentInode->indirBlockPtr = indirectBlockNum;
        DevWriteBlock(bnum, indirectBlock);
        PutInode(parentInodeNum, parentInode);
    }

    int indirectBlock[NUM_OF_BLOCKS_IN_INDIRECTBLOCK];
    DevReadBlock(indirectBlockNum, indirectBlock);

    for (i = 0; i < NUM_OF_BLOCKS_IN_INDIRECTBLOCK; i++) {
        bnum = indirectBlock[i];
        DirEntry de[NUM_OF_DIRENT_PER_BLOCK];

        if (bnum == 0) { //add new block for indirectBlock
            int newBlockNum = GetFreeBlockNum();
            SetBlockBitmap(newBlockNum);
            updateFileSysInfo(ALOCATE_BLOCK);
            parentInode->size += BLOCK_SIZE;
            indirectBlock[i] = newBlockNum;
            DirEntry *newDirEntry = (DirEntry *) malloc(BLOCK_SIZE);
            memset(newDirEntry, 0, BLOCK_SIZE);
            DevWriteBlock(newBlockNum, newDirEntry);
            DevWriteBlock(indirectBlockNum, indirectBlock);
            free(newDirEntry);
        }
        bnum = indirectBlock[i];
        DevReadBlock(bnum, de);

        for (j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (strcmp(de[j].name, "") == 0) { //get empty dirent number
                int newInodeNum = addNewFileBlock(j, target, parentInode, parentInodeNum, bnum, de);
                DevWriteBlock(indirectBlockNum, indirectBlock);
                free(parentInode);
                return newInodeNum;
            }
        }
    }
    free(parentInode);
    return -1;
}

int OpenFile(const char *szFileName, OpenFlag flag) {
    int *curInodeNum = (int *) malloc(sizeof(int));
    *curInodeNum = 0;

    int arr_index = 0;

    char arr[64][MAX_NAME_LEN + 1]; //폴더 갯수 최대 그냥 64로 했음
    char *input_path = (char *) malloc(sizeof(szFileName));
    strcpy(input_path, szFileName);
    char *temp_Ptr = strtok(input_path, "/");

    while (temp_Ptr != NULL) {
        strcpy(arr[arr_index++], temp_Ptr);
        temp_Ptr = strtok(NULL, "/");
    }
    int oh_array_index = 0;
    Inode *pInode = (Inode *) malloc(sizeof(Inode));
    GetInode(0, pInode);
    int *parentBlockNum = (int *) malloc(sizeof(int));

    for (oh_array_index; oh_array_index < arr_index - 1; oh_array_index++) // 루트에 만들어지면 여기는 그냥 통과가 되버리네
    {
        *curInodeNum = goDownDir(arr[oh_array_index], pInode, curInodeNum, parentBlockNum);// 이게 돌아주면 될것 같다
    }
    int parentInodeNum = *curInodeNum;
    *curInodeNum = goDownDir(arr[oh_array_index], pInode, curInodeNum, parentBlockNum);// 이게 돌아주면 될것 같다

    if (flag == OPEN_FLAG_CREATE) {
//        addFileDir(arr[arr_index], parentInodeNum);
        int newInodeNum = addFileDir(arr[arr_index - 1], parentInodeNum);
        if (newInodeNum == -1)
            return -1;
        int returnValue = initNewFile(newInodeNum);
        free(pInode);
        free(curInodeNum);
        free(parentBlockNum);
        return returnValue;
    } else if (flag == OPEN_FLAG_READWRITE) {
        int i;
        for (i = 0; i < MAX_FD_ENTRY_LEN; i++) {
            if (pFileDescTable->file[i].bUsed == 0) {
                pFileDescTable->file[i].bUsed = 1;
                pFileDescTable->file[i].inodeNum = *curInodeNum;
                pFileDescTable->file[i].fileOffset = 0;
                return i;
            }
//            int i;
//            for (i = 0; i < MAX_FD_ENTRY_LEN; i++) {
//                if (pFileDescTable->file[i].bUsed == 0) {
//                    pFileDescTable->file[i].bUsed = 1;
//                    pFileDescTable->file[i].inodeNum = *curInodeNum;
//                    pFileDescTable->file[i].fileOffset = 0;
//                    return i;
//                }
//            }


        }
    }
}

void addBlockInInode(Inode *fInode, int inodeNum, int writeBlockNum) {
    int i;
    int newBlockNum;
    for (i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {
        if (fInode->dirBlockPtr[i] == 0) {
            fInode->dirBlockPtr[i] = writeBlockNum;
            PutInode(inodeNum, fInode);
            return;
        }
    }

    int indirectBlockNum = fInode->indirBlockPtr;
    if (indirectBlockNum == 0) { //does't allocate indirect Block yet
        //add new indirect Block
        newBlockNum = GetFreeBlockNum();
        updateFileSysInfo(ALOCATE_BLOCK);
        SetBlockBitmap(newBlockNum);
        indirectBlockNum = newBlockNum;
        int indirectBlock[NUM_OF_BLOCKS_IN_INDIRECTBLOCK];
        memset(indirectBlock, 0, BLOCK_SIZE);
        fInode->indirBlockPtr = indirectBlockNum;
        DevWriteBlock(indirectBlockNum, indirectBlock);
        PutInode(inodeNum, fInode);
    }
    int indirectBlock[NUM_OF_BLOCKS_IN_INDIRECTBLOCK];
    int bnum;
    DevReadBlock(indirectBlockNum, indirectBlock);

    //link with indirect
    for (i = 0; i < NUM_OF_BLOCKS_IN_INDIRECTBLOCK; i++) {
        bnum = indirectBlock[i];

        if (bnum == 0) { //add new block for indirectBlock
            char *newFileBlock = (char *) malloc(BLOCK_SIZE);
            indirectBlock[i] = writeBlockNum;
            memset(newFileBlock, 0, BLOCK_SIZE);
            DevWriteBlock(writeBlockNum, newFileBlock);
            DevWriteBlock(indirectBlockNum, indirectBlock);
            free(newFileBlock);
            return;
        }
    }
}

int getAvailableByte(int flow) {
    int div = flow / BLOCK_SIZE;
    int totalBlockSize = (div + 1) * BLOCK_SIZE;
    return totalBlockSize - flow;
}

int getFileBlockNum(Inode *fInode, int offset) {
    int div = offset / BLOCK_SIZE;
    if (div == 0)
        return fInode->dirBlockPtr[0];
    if (div == 1)
        return fInode->dirBlockPtr[1];

    int i;
    div -= 2;
    int indirectBlock[NUM_OF_BLOCKS_IN_INDIRECTBLOCK];
    DevReadBlock(fInode->indirBlockPtr, indirectBlock);
    return indirectBlock[div];
}

int WriteFile(int fileDesc, char *pBuffer, int length) {

    if (fileDesc < 0 || fileDesc > MAX_FD_ENTRY_LEN)
        return -1;
    if (pFileDescTable->file[fileDesc].bUsed == false)
        return -1;

    Inode *fInode = (Inode *) malloc(sizeof(Inode));
    int inodeNum = pFileDescTable->file[fileDesc].inodeNum;
    int offset = pFileDescTable->file[fileDesc].fileOffset;
    int stringLength = strlen(pBuffer);

    GetInode(inodeNum, fInode);



    if(offset==0)
    {
        fInode->dirBlockPtr[0]=0;//이거 쓰래기 값 있어서 넣어줌
    }
    


    char *writeBlock = (char *) malloc(BLOCK_SIZE);
    memset(writeBlock, 0, sizeof(BLOCK_SIZE));
    int writeBlockNum = 0;
    int flow = 0;
    int totalWriteSize = 0;
    int i;
    int saveFileBlockNum;

    //TODO : File Write Size
    while (totalWriteSize < length) {
        flow = offset + totalWriteSize;
        //if (flow % BLOCK_SIZE == 0) {//no data or full block
        if (fInode->size % BLOCK_SIZE == 0) { //no data or full block
            writeBlockNum = GetFreeBlockNum();
            DevWriteBlock(writeBlockNum, writeBlock);
            updateFileSysInfo(ALOCATE_BLOCK);
            SetBlockBitmap(writeBlockNum);
            addBlockInInode(fInode, inodeNum, writeBlockNum);
        }
        saveFileBlockNum = getFileBlockNum(fInode, flow);
        char saveBlock[BLOCK_SIZE];
        DevReadBlock(saveFileBlockNum, saveBlock);

        int availableByte = getAvailableByte(flow);
        int writeStartIndex = BLOCK_SIZE - availableByte;
        for (i = 0; i < availableByte; i++) {
            saveBlock[writeStartIndex] = pBuffer[totalWriteSize];
            writeStartIndex++;
            totalWriteSize++;
            if (totalWriteSize >= length || totalWriteSize >= stringLength) {
                DevWriteBlock(saveFileBlockNum, saveBlock);
                break;
            }
        }
        DevWriteBlock(saveFileBlockNum, saveBlock);
        if (totalWriteSize >= length || totalWriteSize >= stringLength)
            break;
    }
    pFileDescTable->file[fileDesc].fileOffset += totalWriteSize;
    fInode->size += totalWriteSize;
    PutInode(inodeNum, fInode);
    free(fInode);
    free(writeBlock);
    return totalWriteSize;
}

int ReadFile(int fileDesc, char *pBuffer, int length) {
    Inode *fInode = (Inode *) malloc(sizeof(Inode));
    int inodeNum = pFileDescTable->file[fileDesc].inodeNum;
    int offset = pFileDescTable->file[fileDesc].fileOffset;
    //int offset = 0;
    GetInode(inodeNum, fInode);
    int size = fInode->size;

    char *readBlock = (char *) malloc(BLOCK_SIZE);
    int readBlockNum = 0;
    int flow = 0;
    int totalReadSize = 0;
    int i;

    //TODO : File Write Link with blocks
    while (totalReadSize < length) {
        flow = offset + totalReadSize;
        if (flow >= size)
            break;
        readBlockNum = getFileBlockNum(fInode, flow);
        DevReadBlock(readBlockNum, readBlock);

        int availableByte = getAvailableByte(flow);
        int readStartIndex = BLOCK_SIZE - availableByte;
        char c;
        for (i = 0; i < availableByte; i++) {
            c = readBlock[readStartIndex];
            if (c == 0)
                break;

            pBuffer[totalReadSize] = c;
            readStartIndex++;
            totalReadSize++;
            if (totalReadSize >= length) {
                break;
            }
        }
        if (totalReadSize >= length)
            break;
    }
    pFileDescTable->file[fileDesc].fileOffset += totalReadSize;
    free(fInode);
    free(readBlock);
    return totalReadSize;
}


int CloseFile(int fileDesc) {

    if (pFileDescTable->file[fileDesc].bUsed == 1) {
        pFileDescTable->file[fileDesc].fileOffset = 0;
        pFileDescTable->file[fileDesc].bUsed = 0;
        pFileDescTable->file[fileDesc].inodeNum = 0;
        return 0;
    }

    return -1;
}

int isOpened(int fileInodeNum) {
    int i;
    for (i = 0; i < MAX_FD_ENTRY_LEN; i++) {
        if (pFileDescTable->file[i].inodeNum == fileInodeNum)
            return 1;
    }
    return 0;
}

void disLinkWithParent(Inode *parentInode, int parentInodeNum, int pInodeNum) {
    int i, j;
    int bnum;

    //direct DE
    DirEntry de[NUM_OF_DIRENT_PER_BLOCK];
    for (i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {
        bnum = parentInode->dirBlockPtr[i];
        DevReadBlock(bnum, de);
        for (j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (de[j].inodeNum == pInodeNum) {
                de[j].inodeNum = 0;
                memset(de[j].name, 0, sizeof(de[j].name));
                //strcpy(de[j].name,"");
                DevWriteBlock(bnum, de);

                //if number of de equal 0, also dislink DE with parent;
                int numOfEmptyBlockInDE = getNumOfEmptyBlockInDE(de);
                if (numOfEmptyBlockInDE == 4) {
                    ResetBlockBitmap(bnum);
                    updateFileSysInfo(FREE_BLOCK);
                    parentInode->dirBlockPtr[i] = 0;
                    parentInode->size -= BLOCK_SIZE;
                    PutInode(parentInodeNum, parentInode);
                }

                return;
            }
        }
    }
    int indirectBlock[NUM_OF_BLOCKS_IN_INDIRECTBLOCK];
    int inbnum;
    bnum = parentInode->indirBlockPtr;
    DevReadBlock(bnum, indirectBlock);
    for (i = 0; i < NUM_OF_BLOCKS_IN_INDIRECTBLOCK; i++) {
        inbnum = indirectBlock[i];
        DevReadBlock(inbnum, de);
        for (j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (de[j].inodeNum == pInodeNum) {
                de[j].inodeNum = 0;
                updateFileSysInfo(FREE_INODE);
                ResetInodeBitmap(pInodeNum);
                memset(de[j].name, 0, sizeof(de[j].name));
                //strcpy(de[j].name,"");
                DevWriteBlock(inbnum, de);

                int numOfEmptyBlockInDE = getNumOfEmptyBlockInDE(de);
                //de has no child, free block
                if (numOfEmptyBlockInDE == 4) {
                    ResetBlockBitmap(inbnum);
                    updateFileSysInfo(FREE_BLOCK);
                    indirectBlock[i] = 0;
                    parentInode->size -= BLOCK_SIZE;
                    DevWriteBlock(bnum, indirectBlock);
                }

                //dislink indirectBLOCK
                int numIndirectDE = getNumOfIndirectDE(indirectBlock);
                if (numIndirectDE == 0) {
                    ResetInodeBitmap(parentInode->indirBlockPtr);
                    updateFileSysInfo(FREE_INODE);
                    parentInode->indirBlockPtr = 0;
                    PutInode(parentInodeNum, parentInode);
                }
                return;
            }
        }
    }
}

int RemoveFile(const char *szFileName) {
//    • 파일을 제거한다. 단, open된 파일을 제거할 수 없다.
//    • Parameters
//    ◦ szFileName[in]: 제거할 파일 이름. 단, 파일 이름은 절대 경로임.
//                                              - Return
//    성공하면, 0를 리턴한다. 실패했을때는 -1을 리턴한다. 실패 원인으로 (1) 제거할 파일 이름이 없을 경우, (2) 제거될 파일이 open되어 있을 경우.
    Inode *pInode = (Inode *) malloc(sizeof(Inode));

    int *curInodeNum = (int *) malloc(sizeof(int));

    *curInodeNum = 0;

    int arr_index = 0;

    char arr[64][MAX_NAME_LEN + 1]; //폴더 갯수 최대 그냥 64로 했음
    char *input_path = (char *) malloc(sizeof(szFileName));
    strcpy(input_path, szFileName);
    char *temp_Ptr = strtok(input_path, "/");

    while (temp_Ptr != NULL) {
        strcpy(arr[arr_index++], temp_Ptr);
        temp_Ptr = strtok(NULL, "/");
    }
    int oh_array_index = 0;
    GetInode(0, pInode);
    int *parentBlockNum = (int *) malloc(sizeof(int));

    for (oh_array_index; oh_array_index < arr_index - 1; oh_array_index++) // 루트에 만들어지면 여기는 그냥 통과가 되버리네
    {
        *curInodeNum = goDownDir(arr[oh_array_index], pInode, curInodeNum, parentBlockNum);// 이게 돌아주면 될것 같다
    }
    int parentInodeNum = *curInodeNum;
    *curInodeNum = goDownDir(arr[oh_array_index], pInode, curInodeNum, parentBlockNum);// 이게 돌아주면 될것 같다
    int i;
    int bnum;

    if (*curInodeNum == -1) {
        free(pInode);
        free(curInodeNum);
        free(parentBlockNum);
        return -1;
    }

    //int fileInodeNum = getFileInodeNum(pInode,target);

    if (isOpened(*curInodeNum) == 1) { // file has been opened
        free(pInode);
        free(curInodeNum);
        free(parentBlockNum);
        return -1;
    }

    Inode *fInode = (Inode *) malloc(sizeof(Inode));
    GetInode(*curInodeNum, fInode);

    char *pBlock = (char *) malloc(BLOCK_SIZE);
    for (i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {
        bnum = fInode->dirBlockPtr[i];
        if (bnum == 0)
            continue;
        DevReadBlock(bnum, pBlock);
        memset(pBlock, 0, BLOCK_SIZE);
        DevWriteBlock(bnum, pBlock);
        ResetBlockBitmap(bnum);
        updateFileSysInfo(FREE_BLOCK);
        fInode->dirBlockPtr[i] = 0;
    }
    bnum = fInode->indirBlockPtr;
    if (bnum != 0) {
        int inbnum;
        int indirectBlock[NUM_OF_BLOCKS_IN_INDIRECTBLOCK];
        DevReadBlock(bnum, indirectBlock);
        for (i = 0; i < NUM_OF_BLOCKS_IN_INDIRECTBLOCK; i++) {
            inbnum = indirectBlock[i];
            if (inbnum == 0)
                continue;
            DevReadBlock(inbnum, pBlock);
            memset(pBlock, 0, BLOCK_SIZE);
            DevWriteBlock(inbnum, pBlock);
            ResetBlockBitmap(inbnum);
            updateFileSysInfo(FREE_BLOCK);
        }
        DevReadBlock(bnum, pBlock);
        memset(pBlock, 0, BLOCK_SIZE);
        DevWriteBlock(bnum, pBlock);
        ResetBlockBitmap(bnum);
        updateFileSysInfo(FREE_BLOCK);
        fInode->indirBlockPtr = 0;
    }
    //free inode,
    ResetInodeBitmap(*curInodeNum);
    updateFileSysInfo(FREE_INODE);

    fInode->size = 0;
    fInode->type = 0;

    Inode *parentInode = (Inode *) malloc(BLOCK_SIZE);
    GetInode(parentInodeNum, parentInode);
    disLinkWithParent(parentInode, parentInodeNum, *curInodeNum);
    PutInode(parentInodeNum, parentInode);
    PutInode(*curInodeNum, pInode);
    //dislink with parent

    free(pInode);
    free(curInodeNum);
    free(parentBlockNum);
    free(fInode);
    free(pBlock);
    free(parentInode);
    return 0;
}


int MakeDir(const char *szDirName) {

    int *curInodeNum = (int *) malloc(sizeof(int));
    *curInodeNum = 0;

    int arr_index = 0;

    char arr[64][MAX_NAME_LEN + 1]; //폴더 갯수 최대 그냥 64로 했음
    char *input_path = (char *) malloc(sizeof(szDirName));
    strcpy(input_path, szDirName);
    char *temp_Ptr = strtok(input_path, "/");

    while (temp_Ptr != NULL) {
        strcpy(arr[arr_index++], temp_Ptr);
        temp_Ptr = strtok(NULL, "/");
    }
    int oh_array_index = 0;
    Inode *pInode = (Inode *) malloc(sizeof(Inode));
    GetInode(0, pInode);
    int *parentBlockNum = (int *) malloc(sizeof(int));

    for (oh_array_index; oh_array_index < arr_index - 1; oh_array_index++) // 루트에 만들어지면 여기는 그냥 통과가 되버리네
    {
        *curInodeNum = goDownDir(arr[oh_array_index], pInode, curInodeNum, parentBlockNum);// 이게 돌아주면 될것 같다
    }
    int parentInodeNum = *curInodeNum;
    int i, j;
    int bnum;//이거 왜 1이 들어가는거야 시발
    int newBlockNum = -1;
    DirEntry de[NUM_OF_DIRENT_PER_BLOCK];
    memset(de, 0, BLOCK_SIZE);



    //travel direct pointers
    for (i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {

        bnum = pInode->dirBlockPtr[i];
        if (bnum == 0) { //add new direct pointer and new entry
            int newBlockNum = addNewDirEntry(i, pInode);
            pInode->dirBlockPtr[i] = newBlockNum;
            pInode->size += BLOCK_SIZE;
            bnum = pInode->dirBlockPtr[i];
            PutInode(parentInodeNum, pInode);
        }
        DevReadBlock(bnum, de);

        //add new dir
        for (j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) { //4
            if (strcmp(de[j].name, "") == 0) {
                newBlockNum = addNewDirBlock(arr[arr_index - 1], parentInodeNum, bnum, de);
                return 0;
            }
        }
    }

    //--------indirect------

    //travel indirect block
    int indirectBlockNum = pInode->indirBlockPtr;
    if (indirectBlockNum == 0) { //does't allocate indirect Block yet
        //add new indirect Block
        newBlockNum = GetFreeBlockNum();
        updateFileSysInfo(ALOCATE_BLOCK);
        SetBlockBitmap(newBlockNum);
        indirectBlockNum = newBlockNum;
        bnum = newBlockNum;
        int indirectBlock[NUM_OF_BLOCKS_IN_INDIRECTBLOCK];
        memset(indirectBlock, 0, BLOCK_SIZE);
        pInode->indirBlockPtr = indirectBlockNum;
        DevWriteBlock(bnum, indirectBlock);
        PutInode(parentInodeNum, pInode);
    }

    int indirectBlock[NUM_OF_BLOCKS_IN_INDIRECTBLOCK];
    DevReadBlock(indirectBlockNum, indirectBlock);

    for (i = 0; i < NUM_OF_BLOCKS_IN_INDIRECTBLOCK; i++) {
        bnum = indirectBlock[i];
        DirEntry de[NUM_OF_DIRENT_PER_BLOCK];

        if (bnum == 0) { //add new block for indirectBlock
            int newBlockNum = GetFreeBlockNum();
            SetBlockBitmap(newBlockNum);
            updateFileSysInfo(ALOCATE_BLOCK);
            pInode->size += BLOCK_SIZE;
            indirectBlock[i] = newBlockNum;
            DirEntry *newDirEntry = (DirEntry *) malloc(BLOCK_SIZE);
            memset(newDirEntry, 0, BLOCK_SIZE);
            DevWriteBlock(newBlockNum, newDirEntry);
            DevWriteBlock(indirectBlockNum, indirectBlock);
            free(newDirEntry);
        }
        bnum = indirectBlock[i];
        DevReadBlock(bnum, de);

        for (j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (strcmp(de[j].name, "") == 0) { //get empty dirent number
                newBlockNum = addNewDirBlock(arr[arr_index - 1], parentInodeNum, bnum, de);
                DevWriteBlock(indirectBlockNum, indirectBlock);
                return 0;
            }
        }
    }
    return -1;

}


int getNumOfEmptyBlockInDE(DirEntry *de) {
    int i = 0;
    int count = 0;
    for (i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++) {
        if (de[i].inodeNum == 0)
            count++;
    }
    return count;
}

int getNumOfIndirectDE(int indirectBlock[]) {
    int i;
    int count = 0;
    for (i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
        if (indirectBlock[i] != 0) {
            count++;
        }
    }
    return count;
}

int RemoveDir(const char *szDirName) {
    int *curInodeNum = (int *) malloc(sizeof(int));
    *curInodeNum = 0;

    int arr_index = 0;

    char arr[64][MAX_NAME_LEN + 1]; //폴더 갯수 최대 그냥 64로 했음
    char *input_path = (char *) malloc(sizeof(szDirName));
    strcpy(input_path, szDirName);
    char *temp_Ptr = strtok(input_path, "/");

    while (temp_Ptr != NULL) {
        strcpy(arr[arr_index++], temp_Ptr);
        temp_Ptr = strtok(NULL, "/");
    }
    int oh_array_index = 0;
    Inode *pInode = (Inode *) malloc(sizeof(Inode));
    GetInode(0, pInode);
    int *parentBlockNum = (int *) malloc(sizeof(int));

    for (oh_array_index; oh_array_index < arr_index - 1; oh_array_index++) // 루트에 만들어지면 여기는 그냥 통과가 되버리네
    {
        *curInodeNum = goDownDir(arr[oh_array_index], pInode, curInodeNum, parentBlockNum);// 이게 돌아주면 될것 같다
    }
    int parentInodeNum = *curInodeNum;
    *curInodeNum = goDownDir(arr[oh_array_index], pInode, curInodeNum, parentBlockNum);// 이게 돌아주면 될것 같다
    if (*curInodeNum == -1) //no such directory
        return -1;

    char *pBlock = (char *) malloc(BLOCK_SIZE);

    ResetBlockBitmap(pInode->dirBlockPtr[0]);
    ResetInodeBitmap(*curInodeNum);
    updateFileSysInfo(FREE_INODE);
    updateFileSysInfo(FREE_BLOCK);
    Inode *parentInode = (Inode *) malloc(BLOCK_SIZE);
    GetInode(parentInodeNum, parentInode);
    //disLinkWithParent(parentInode, *curInodeNum, *curInodeNum);

    int i, j;
    int bnum;

    //direct DE
    DirEntry de[NUM_OF_DIRENT_PER_BLOCK];
    for (i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {
        bnum = parentInode->dirBlockPtr[i];
        DevReadBlock(bnum, de);
        for (j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (de[j].inodeNum == *curInodeNum) {
                de[j].inodeNum = 0;
                memset(de[j].name, 0, sizeof(de[j].name));
                //strcpy(de[j].name,"");
                DevWriteBlock(bnum, (char *) de);

                //if number of de equal 0, also dislink DE with parent;
                int numOfEmptyBlockInDE = getNumOfEmptyBlockInDE(de);
                if (numOfEmptyBlockInDE == 4) {
                    ResetBlockBitmap(bnum);
                    updateFileSysInfo(FREE_BLOCK);
                    parentInode->dirBlockPtr[i] = 0;
                    parentInode->size -= BLOCK_SIZE;
                    PutInode(parentInodeNum, parentInode);
                }

                return 0;
            }
        }
    }
    int indirectBlock[BLOCK_SIZE / sizeof(int)];
    int inbnum;
    bnum = parentInode->indirBlockPtr;
    DevReadBlock(bnum, (char *) indirectBlock);
    for (i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
        inbnum = indirectBlock[i];
        DevReadBlock(inbnum, (char *) de);
        for (j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (de[j].inodeNum == *curInodeNum) {
                de[j].inodeNum = 0;
                updateFileSysInfo(FREE_INODE);
                ResetInodeBitmap(*curInodeNum);
                memset(de[j].name, 0, sizeof(de[j].name));
                //strcpy(de[j].name,"");
                DevWriteBlock(inbnum, de);

                int numOfEmptyBlockInDE = getNumOfEmptyBlockInDE(de);
                //de has no child, free block
                if (numOfEmptyBlockInDE == 4) {
                    ResetBlockBitmap(inbnum);
                    updateFileSysInfo(FREE_BLOCK);
                    indirectBlock[i] = 0;
                    parentInode->size -= BLOCK_SIZE;
                    DevWriteBlock(bnum, (char *) indirectBlock);
                }

                //dislink indirectBLOCK
                int numIndirectDE = getNumOfIndirectDE(indirectBlock);
                if (numIndirectDE == 0) {
                    ResetInodeBitmap(parentInode->indirBlockPtr);
                    updateFileSysInfo(FREE_INODE);
                    parentInode->indirBlockPtr = 0;
                    PutInode(parentInodeNum, parentInode);
                }
                return 0;
            }
        }
    }


    free(pInode);
    free(curInodeNum);
    free(parentBlockNum);
    free(pBlock);
    free(parentInode);
    return 0;

}

int EnumerateBlocks(DirEntry *de, DirEntryInfo *pDirEntry, int dirEntrys, int count) {
    int i;
    Inode *tempInode = (Inode *) malloc(sizeof(Inode));

    for (i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++) {
        if (count >= dirEntrys)
            break;
        if (strcmp(de[i].name, "") == 0)
            continue;
        pDirEntry[count].inodeNum = de[i].inodeNum;
        strcpy(pDirEntry[count].name, de[i].name);
        GetInode(de[i].inodeNum, tempInode);
        pDirEntry[count].type = tempInode->type;
        count++;

    }
    free(tempInode);
    return count;
}

int EnumerateDirStatus(const char *szDirName, DirEntryInfo *pDirEntry, int dirEntrys) {
    Inode *pInode = (Inode *) malloc(sizeof(Inode));
    int *curInodeNum = (int *) malloc(sizeof(int));
    *curInodeNum = 0;
    int *parentBlockNum = (int *) malloc(sizeof(int));

    int arr_index = 0;

    char arr[64][MAX_NAME_LEN + 1]; //폴더 갯수 최대 그냥 64로 했음
    char *input_path = (char *) malloc(sizeof(szDirName));
    strcpy(input_path, szDirName);
    char *temp_Ptr = strtok(input_path, "/");

    while (temp_Ptr != NULL) {
        strcpy(arr[arr_index++], temp_Ptr);
        temp_Ptr = strtok(NULL, "/");
    }
    int oh_array_index = 0;
    GetInode(0, pInode);
    for (oh_array_index; oh_array_index < arr_index - 1; oh_array_index++) // 루트에 만들어지면 여기는 그냥 통과가 되버리네
    {
        *curInodeNum = goDownDir(arr[oh_array_index], pInode, curInodeNum, parentBlockNum);// 이게 돌아주면 될것 같다
    }
    int parentInodeNum = *curInodeNum;
    *curInodeNum = goDownDir(arr[oh_array_index], pInode, curInodeNum, parentBlockNum);// 이게 돌아주면 될것 같다
    if (parentInodeNum == -1 || *curInodeNum == -1) {
        free(pInode);
        free(curInodeNum);
        free(parentBlockNum);
        return -1;
    }

    Inode *parentInode = (Inode *) malloc(sizeof(Inode));
    GetInode(parentInodeNum, parentInode);
    int i, count = 0;
    int bnum;

    //direct
    DirEntry de[NUM_OF_DIRENT_PER_BLOCK];
    for (i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {
        bnum = pInode->dirBlockPtr[i];
        if (bnum == 0)
            continue;
        DevReadBlock(bnum, (char *) de);
        Inode *tempInode = (Inode *) malloc(sizeof(Inode));
        count = EnumerateBlocks(de, pDirEntry, dirEntrys, count);

        free(tempInode);
        if (count >= dirEntrys) {

            free(pInode);
            free(curInodeNum);
            free(parentBlockNum);
            free(parentInode);
            return count;
        }
    }

    //indirect
    int inbnum = pInode->indirBlockPtr;
    if (inbnum != 0) {
        int indirectBlock[BLOCK_SIZE / sizeof(int)];
        DevReadBlock(inbnum, (char *) indirectBlock);
        for (i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
            bnum = indirectBlock[i];
            if (bnum == 0)
                continue;
            DevReadBlock(bnum, de);
            count = EnumerateBlocks(de, pDirEntry, dirEntrys, count);
            if (count >= dirEntrys) {
                free(pInode);
                free(curInodeNum);
                free(parentBlockNum);
                free(parentInode);
                return count;
            }
        }
    }
    free(pInode);
    free(curInodeNum);
    free(parentBlockNum);
    free(parentInode);
    return count;
}


void FileSysInit(void) //Success
{
    DevCreateDisk();

    char *buf = malloc(BLOCK_SIZE);

    memset(buf, 0, BLOCK_SIZE); // memset을 통해서 모든 메모리를 0으로 만듭니다.

    for (int i = 0; i < 512; i++) //512 블록까지 초기화 하라는건가?
    {
        DevWriteBlock(i, buf);
    }
    free(buf);
    if (pFileDescTable == NULL) { //초기화
        pFileDescTable = (FileDescTable *) malloc(sizeof(FileDescTable));
        memset(pFileDescTable, 0, sizeof(FileDescTable));
    }
}

void SetInodeBitmap(int inodeno) {
    char *buf = malloc(BLOCK_SIZE);
    //DevOpenDisk();
    DevReadBlock(INODE_BITMAP_BLOCK_NUM, buf);
    buf[inodeno / 8] |= 1 << (7 - (inodeno % 8)); // if(inodeno % 8 ==0) -> buf[inodeno/8] |= 1000000
    DevWriteBlock(INODE_BITMAP_BLOCK_NUM, buf);
    free(buf);
}

void ResetInodeBitmap(int inodeno) {
    char *buf = malloc(BLOCK_SIZE);
    //DevOpenDisk();
    DevReadBlock(INODE_BITMAP_BLOCK_NUM, buf);
    buf[inodeno / 8] &= ~(1 << (7 - (inodeno % 8)));
    DevWriteBlock(INODE_BITMAP_BLOCK_NUM, buf);
    free(buf);
}

void SetBlockBitmap(int blkno) {

    char *buf = malloc(BLOCK_SIZE);
    //DevOpenDisk();
    DevReadBlock(BLOCK_BITMAP_BLOCK_NUM, buf);
    buf[blkno / 8] |= 1 << (7 - (blkno % 8));
    DevWriteBlock(BLOCK_BITMAP_BLOCK_NUM, buf);
    free(buf); // 왜 세그멘트 나오지
}

void ResetBlockBitmap(int blkno) {
    char *buf = malloc(BLOCK_SIZE);
    //DevOpenDisk();
    DevReadBlock(BLOCK_BITMAP_BLOCK_NUM, buf);
    buf[blkno / 8] &= ~(1 << (7 - (blkno % 8)));
    DevWriteBlock(BLOCK_BITMAP_BLOCK_NUM, buf);
    free(buf);
}

void PutInode(int inodeno, Inode *pInode) {

    //DevOpenDisk();
    char *buf = malloc(BLOCK_SIZE);
    DevReadBlock((INODELIST_BLOCK_FIRST + inodeno / (BLOCK_SIZE / sizeof(Inode))), buf);
    memcpy(buf + ((inodeno % (BLOCK_SIZE / sizeof(Inode))) * sizeof(Inode)), pInode, sizeof(Inode));
    DevWriteBlock((INODELIST_BLOCK_FIRST + inodeno / (BLOCK_SIZE / sizeof(Inode))), buf);
    free(buf);
}

void GetInode(int inodeno, Inode *pInode) {

    //DevOpenDisk();

    char *buf = (char *) malloc(BLOCK_SIZE);

    DevReadBlock(INODELIST_BLOCK_FIRST + inodeno / (BLOCK_SIZE / sizeof(Inode)), buf);
    memcpy(pInode, buf + (inodeno % (BLOCK_SIZE / sizeof(Inode)) * sizeof(Inode)), sizeof(Inode));

    free(buf);
}

int GetFreeInodeNum(void) {
    char *buf = malloc(BLOCK_SIZE);
    ////DevOpenDisk();
    DevReadBlock(INODE_BITMAP_BLOCK_NUM, buf);

    for (int i = 0; i < BLOCK_SIZE; i++) //이러면 24부터네
    {
        if (buf[i] == -1) // if 11111111 => continue
            continue;
        for (int index_direct = 0; index_direct < 8; index_direct++) {
            if ((buf[i] << index_direct & 128) == 0) // >> 연산자가 & 보다 우선순위가 높구나
            {
                free(buf);
                return (i * 8) + index_direct;
            }
        }
    }
    free(buf);
    return -1; //실패 했을 경우
}

int GetFreeBlockNum(void) {
    char *buf = malloc(BLOCK_SIZE);
    DevReadBlock(BLOCK_BITMAP_BLOCK_NUM, buf);
    int i = 2; // 19부터 하기 위함
    for (int index_direct = 3; index_direct < 8; index_direct++) {
        if ((buf[i] << index_direct & 128) == 0) // >> 연산자가 & 보다 우선순위가 높구나
        {
            free(buf);
            return (i * 8) + index_direct;
        }
    }
    for (int i = 3; i < BLOCK_SIZE; i++) {
        if (buf[i] == -1) // if 11111111 => continue
            continue;

        for (int index_direct = 0; index_direct < 8; index_direct++) {
            if ((buf[i] << index_direct & 128) == 0) // >> 연산자가 & 보다 우선순위가 높구나
            {
                free(buf);
                return (i * 8) + index_direct;
            }
        }
    }
    free(buf);
    return -1; //실패 했을 경우
}
