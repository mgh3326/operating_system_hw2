//-fs.c: 과제1에서 구현한 코드를 이 파일에 넣고, 빈 함수를 구현하면 됩니다.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "disk.h"
#include "fs.h"


FileDescTable *pFileDescTable = NULL;

typedef enum _UPDATE_FLAG {
    ADD_DIR,
    ALOCATE_BLOCK,
    ALOCATE_INODE,
    FREE_BLOCK,
    FREE_INODE
} UPDATE_FLAG;

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

int goDownDir(char *find_name, Inode *current_inode, int *current_inode_num, int *block_number) {//이거는 많이 쓸거 같다

    DirEntry dir_entry[NUM_OF_DIRENT_PER_BLOCK];
    for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {
        DevReadBlock(current_inode->dirBlockPtr[i], (char *) dir_entry);
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (strcmp(dir_entry[j].name, find_name) == 0) {
                *current_inode_num = dir_entry[j].inodeNum;
                *block_number = current_inode->dirBlockPtr[i];
                GetInode(dir_entry[j].inodeNum, current_inode);
                return *current_inode_num;
            }
        }
    }
    int indirectBlock[BLOCK_SIZE / sizeof(int)];
    DevReadBlock(current_inode->indirBlockPtr, (char *) indirectBlock);
    for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
        DevReadBlock(indirectBlock[i], (char *) dir_entry);
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (strcmp(dir_entry[j].name, find_name) == 0) {
                *current_inode_num = dir_entry[j].inodeNum;
                *block_number = indirectBlock[i];
                GetInode(dir_entry[j].inodeNum, current_inode);
                return *current_inode_num;
            }
        }
    }
    return -1;
}

int addNewFileBlock(char *find_name, int *parentBlockNum,
                    DirEntry *parentDE) {


    int i;
    for (i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++) {
        if (strcmp(parentDE[i].name, "") == 0) {

            //save child DE
            int newInodeNum = GetFreeInodeNum();
            updateFileSysInfo(ALOCATE_INODE);
            SetInodeBitmap(newInodeNum);
            Inode *newInode = (Inode *) malloc(BLOCK_SIZE);
            memset(newInode, 0, sizeof(newInode));//memory init !!!
            newInode->type = FILE_TYPE_FILE;
            newInode->size = 0;

            PutInode(newInodeNum, newInode);

            //save parent DE
            strcpy(parentDE[i].name, find_name);
            parentDE[i].inodeNum = newInodeNum;
            DevWriteBlock((int) parentBlockNum, (char *) parentDE);

            free(newInode);

            return newInodeNum;
        }
    }
    return -1;
}

int addFileDir(char *find_name, int parentInodeNum);

int addFileDir(char *find_name, int parentInodeNum) {

    int block_number;
    int newBlockNum;
    Inode *parentInode;
    parentInode = (Inode *) malloc(BLOCK_SIZE);
    GetInode(parentInodeNum, parentInode);
    DirEntry dir_entry[NUM_OF_DIRENT_PER_BLOCK];

//    if (isExistInDir(find_name, parentInode) == 1)// //필요 없을거 같
//    { //there is same name file
//        free(parentInode);
//        return -1;
//    }

    //travel direct pointers
    for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {

        block_number = parentInode->dirBlockPtr[i];
        if (block_number == 0) { //add new direct pointer and new entry

            newBlockNum = GetFreeBlockNum();
            updateFileSysInfo(ALOCATE_BLOCK);
            SetBlockBitmap(newBlockNum);

            //init new Entry Block
            DirEntry newDirEntry[NUM_OF_DIRENT_PER_BLOCK];
            memset(newDirEntry, 0, BLOCK_SIZE);

            //link with parent Block
            parentInode->dirBlockPtr[i] = newBlockNum; //ex) 19

            DevWriteBlock(newBlockNum, (char *) newDirEntry);


            parentInode->dirBlockPtr[i] = newBlockNum;
            parentInode->size += BLOCK_SIZE;
            block_number = parentInode->dirBlockPtr[i];
            PutInode(parentInodeNum, parentInode);
        }
        DevReadBlock(block_number, dir_entry);

        //add new dir
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) { //4
            if (strcmp(dir_entry[j].name, "") == 0) {
                int newInodeNum;
                newInodeNum = addNewFileBlock(find_name, block_number, dir_entry);
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
        block_number = newBlockNum;
        int indirectBlock[BLOCK_SIZE / sizeof(int)];
        memset(indirectBlock, 0, BLOCK_SIZE);
        parentInode->indirBlockPtr = indirectBlockNum;
        DevWriteBlock(block_number, (char *) indirectBlock);
        PutInode(parentInodeNum, parentInode);
    }

    int indirectBlock[BLOCK_SIZE / sizeof(int)];
    DevReadBlock(indirectBlockNum, indirectBlock);

    for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
        block_number = indirectBlock[i];
        DirEntry dir_entry[NUM_OF_DIRENT_PER_BLOCK];

        if (block_number == 0) { //add new block for indirectBlock
            newBlockNum = GetFreeBlockNum();
            SetBlockBitmap(newBlockNum);
            updateFileSysInfo(ALOCATE_BLOCK);
            parentInode->size += BLOCK_SIZE;
            indirectBlock[i] = newBlockNum;
            DirEntry *newDirEntry = (DirEntry *) malloc(BLOCK_SIZE);
            memset(newDirEntry, 0, BLOCK_SIZE);
            DevWriteBlock(newBlockNum, (char *) newDirEntry);
            DevWriteBlock(indirectBlockNum, (char *) indirectBlock);
            free(newDirEntry);
        }
        block_number = indirectBlock[i];
        DevReadBlock(block_number, dir_entry);

        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (strcmp(dir_entry[j].name, "") == 0) { //get empty dirent number
                int newInodeNum;
                newInodeNum = addNewFileBlock(find_name, (int *) block_number, dir_entry);
                DevWriteBlock(indirectBlockNum, (char *) indirectBlock);
                free(parentInode);
                return newInodeNum;
            }
        }
    }
    free(parentInode);
    return -1;
}

int OpenFile(const char *szFileName, OpenFlag flag) {
    int *current_inode_num;
    current_inode_num = (int *) malloc(sizeof(int));
    *current_inode_num = 0;

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
        goDownDir(arr[oh_array_index], pInode, current_inode_num, parentBlockNum);// 이게 돌아주면 될것 같다
    }
    int parentInodeNum = *current_inode_num;

//파일이 있다고 가정하에 하는거여서 문제가 발생하는듯 하다.
    if (flag == OPEN_FLAG_CREATE) {
//        addFileDir(arr[arr_index], parentInodeNum);
        int newInodeNum;
        newInodeNum = addFileDir(arr[arr_index - 1], parentInodeNum);
        if (newInodeNum == -1)
            return -1;
        //int returnValue = initNewFile(newInodeNum);

        for (int i = 0; i < MAX_FD_ENTRY_LEN; i++) {
            if (pFileDescTable->file[i].bUsed == 0) {
                pFileDescTable->file[i].bUsed = 1;
                pFileDescTable->file[i].inodeNum = newInodeNum;
                pFileDescTable->file[i].fileOffset = 0;
                free(pInode);
                free(current_inode_num);
                free(parentBlockNum);
                return i;
            }
        }
        return -1;

    } else if (flag == OPEN_FLAG_READWRITE) {
        goDownDir(arr[oh_array_index], pInode, current_inode_num, parentBlockNum);// 이게 돌아주면 될것 같다

        int i;
        for (i = 0; i < MAX_FD_ENTRY_LEN; i++) {
            if (pFileDescTable->file[i].bUsed == 0) {
                pFileDescTable->file[i].bUsed = 1;
                pFileDescTable->file[i].inodeNum = *current_inode_num;
                pFileDescTable->file[i].fileOffset = 0;
                return i;
            }
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
        int indirectBlock[BLOCK_SIZE / sizeof(int)];
        memset(indirectBlock, 0, BLOCK_SIZE);
        fInode->indirBlockPtr = indirectBlockNum;
        DevWriteBlock(indirectBlockNum, (char *) indirectBlock);
        PutInode(inodeNum, fInode);
    }
    int indirectBlock[BLOCK_SIZE / sizeof(int)];
    int block_number;
    DevReadBlock(indirectBlockNum, (char *) indirectBlock);

    //link with indirect
    for (i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
        block_number = indirectBlock[i];

        if (block_number == 0) { //add new block for indirectBlock
            char *newFileBlock = (char *) malloc(BLOCK_SIZE);
            indirectBlock[i] = writeBlockNum;
            memset(newFileBlock, 0, BLOCK_SIZE);
            DevWriteBlock(writeBlockNum, newFileBlock);
            DevWriteBlock(indirectBlockNum, (char *) indirectBlock);
            free(newFileBlock);
            return;
        }
    }
}


int WriteFile(int fileDesc, char *pBuffer, int length) {

    if (fileDesc < 0 || fileDesc > MAX_FD_ENTRY_LEN)
        return -1;
    if (pFileDescTable->file[fileDesc].bUsed == 0)
        return -1;

    Inode *fInode;
    fInode = (Inode *) malloc(sizeof(Inode));
    int inodeNum;
    inodeNum = pFileDescTable->file[fileDesc].inodeNum;
    int offset = pFileDescTable->file[fileDesc].fileOffset;
    int stringLength = (int) strlen(pBuffer);

    GetInode(inodeNum, fInode);


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
        if (flow / BLOCK_SIZE == 0)
            saveFileBlockNum = fInode->dirBlockPtr[0];
        else if (flow / BLOCK_SIZE == 1)
            saveFileBlockNum = fInode->dirBlockPtr[1];
        else {
            int indirectBlock[BLOCK_SIZE / sizeof(int)];
            DevReadBlock(fInode->indirBlockPtr, (char *) indirectBlock);
            saveFileBlockNum = indirectBlock[0];

        }
        char saveBlock[BLOCK_SIZE];
        DevReadBlock(saveFileBlockNum, saveBlock);

        int availableByte = ((flow / BLOCK_SIZE) + 1) * BLOCK_SIZE - flow;

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
    Inode *fInode;
    fInode = (Inode *) malloc(sizeof(Inode));

    GetInode(pFileDescTable->file[fileDesc].inodeNum, fInode);
    int size = fInode->size;

    char *readBlock = (char *) malloc(BLOCK_SIZE);

    int totalReadSize = 0;

    //TODO : File Write Link with blocks
    while (totalReadSize < length) {
        int flow = pFileDescTable->file[fileDesc].fileOffset + totalReadSize;
        if (flow >= size)
            break;
        int readBlockNum;
        if (flow / BLOCK_SIZE == 0)
            readBlockNum = fInode->dirBlockPtr[0];
        else if (flow / BLOCK_SIZE == 1)
            readBlockNum = fInode->dirBlockPtr[1];
        else {
            int indirectBlock[BLOCK_SIZE / sizeof(int)];
            DevReadBlock(fInode->indirBlockPtr, (char *) indirectBlock);
            readBlockNum = indirectBlock[0];

        }

        DevReadBlock(readBlockNum, readBlock);

        int availableByte = ((flow / BLOCK_SIZE) + 1) * BLOCK_SIZE - flow;
        int readStartIndex = BLOCK_SIZE - availableByte;
        char c;
        for (int i = 0; i < availableByte; i++) {
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

    if (pFileDescTable->file[fileDesc].bUsed != 0) {
        pFileDescTable->file[fileDesc].inodeNum = 0;
        pFileDescTable->file[fileDesc].fileOffset = 0;
        pFileDescTable->file[fileDesc].bUsed = 0;
        return 0;
    }

    return -1;
}


int RemoveFile(const char *szFileName) {
//    • 파일을 제거한다. 단, open된 파일을 제거할 수 없다.
//    • Parameters
//    ◦ szFileName[in]: 제거할 파일 이름. 단, 파일 이름은 절대 경로임.
//                                              - Return
//    성공하면, 0를 리턴한다. 실패했을때는 -1을 리턴한다. 실패 원인으로 (1) 제거할 파일 이름이 없을 경우, (2) 제거될 파일이 open되어 있을 경우.
    Inode *pInode = (Inode *) malloc(sizeof(Inode));

    int *current_inode_num = (int *) malloc(sizeof(int));

    *current_inode_num = 0;

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
        goDownDir(arr[oh_array_index], pInode, current_inode_num, parentBlockNum);// 이게 돌아주면 될것 같다
    }
    int parentInodeNum = *current_inode_num;
    goDownDir(arr[oh_array_index], pInode, current_inode_num, parentBlockNum);// 이게 돌아주면 될것 같다
    int block_number;

    if (*current_inode_num == -1) {
        free(pInode);
        free(current_inode_num);
        free(parentBlockNum);
        return -1;
    }

    //int fileInodeNum = getFileInodeNum(pInode,find_name);


    for (int i = 0; i < MAX_FD_ENTRY_LEN; i++) {
        if (pFileDescTable->file[i].inodeNum == *current_inode_num)
            free(pInode);
        free(current_inode_num);
        free(parentBlockNum);
        return -1;
    }


    Inode *fInode = (Inode *) malloc(sizeof(Inode));
    GetInode(*current_inode_num, fInode);

    char *pBlock = (char *) malloc(BLOCK_SIZE);
    for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {
        block_number = fInode->dirBlockPtr[i];
        if (block_number == 0)
            continue;
        DevReadBlock(block_number, pBlock);
        memset(pBlock, 0, BLOCK_SIZE);
        DevWriteBlock(block_number, pBlock);
        ResetBlockBitmap(block_number);
        updateFileSysInfo(FREE_BLOCK);
        fInode->dirBlockPtr[i] = 0;
    }
    block_number = fInode->indirBlockPtr;
    if (block_number != 0) {
        int inode_block_num;
        int indirectBlock[BLOCK_SIZE / sizeof(int)];
        DevReadBlock(block_number, indirectBlock);
        for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
            inode_block_num = indirectBlock[i];
            if (inode_block_num == 0)
                continue;
            DevReadBlock(inode_block_num, pBlock);
            memset(pBlock, 0, BLOCK_SIZE);
            DevWriteBlock(inode_block_num, pBlock);
            ResetBlockBitmap(inode_block_num);
            updateFileSysInfo(FREE_BLOCK);
        }
        DevReadBlock(block_number, pBlock);
        memset(pBlock, 0, BLOCK_SIZE);
        DevWriteBlock(block_number, pBlock);
        ResetBlockBitmap(block_number);
        updateFileSysInfo(FREE_BLOCK);
        fInode->indirBlockPtr = 0;
    }
    //free inode,
    ResetInodeBitmap(*current_inode_num);
    updateFileSysInfo(FREE_INODE);

    fInode->size = 0;
    fInode->type = 0;

    Inode *parentInode = (Inode *) malloc(BLOCK_SIZE);
    GetInode(parentInodeNum, parentInode);

    //disLinkWithParent(parentInode, parentInodeNum, *current_inode_num);

    DirEntry dir_entry[NUM_OF_DIRENT_PER_BLOCK];
    for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {
        block_number = parentInode->dirBlockPtr[i];
        DevReadBlock(block_number, dir_entry);
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (dir_entry[j].inodeNum == *current_inode_num) {
                dir_entry[j].inodeNum = 0;
                memset(dir_entry[j].name, 0, sizeof(dir_entry[j].name));
                //strcpy(dir_entry[j].name,"");
                DevWriteBlock(block_number, dir_entry);

                //if number of dir_entry equal 0, also dislink DE with parent;
                int numOfEmptyBlockInDE = 0;
                for (int i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++) {
                    if (dir_entry[i].inodeNum == 0)
                        numOfEmptyBlockInDE++;
                }
                if (numOfEmptyBlockInDE == 4) {
                    ResetBlockBitmap(block_number);
                    updateFileSysInfo(FREE_BLOCK);
                    parentInode->dirBlockPtr[i] = 0;
                    parentInode->size -= BLOCK_SIZE;
                    PutInode(parentInodeNum, parentInode);
                }


                PutInode(parentInodeNum, parentInode);
                PutInode(*current_inode_num, pInode);
                //dislink with parent

                free(pInode);
                free(current_inode_num);
                free(parentBlockNum);
                free(fInode);
                free(pBlock);
                free(parentInode);
                return 0;
            }
        }
    }
    int indirectBlock[BLOCK_SIZE / sizeof(int)];
    int inode_block_num;
    block_number = parentInode->indirBlockPtr;
    DevReadBlock(block_number, indirectBlock);
    for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
        inode_block_num = indirectBlock[i];
        DevReadBlock(inode_block_num, dir_entry);
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (dir_entry[j].inodeNum == *current_inode_num) {
                dir_entry[j].inodeNum = 0;
                updateFileSysInfo(FREE_INODE);
                ResetInodeBitmap(*current_inode_num);
                memset(dir_entry[j].name, 0, sizeof(dir_entry[j].name));
                //strcpy(dir_entry[j].name,"");
                DevWriteBlock(inode_block_num, dir_entry);

                int numOfEmptyBlockInDE = 0;
                for (int i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++) {
                    if (dir_entry[i].inodeNum == 0)
                        numOfEmptyBlockInDE++;
                }                //dir_entry has no child, free block
                if (numOfEmptyBlockInDE == 4) {
                    ResetBlockBitmap(inode_block_num);
                    updateFileSysInfo(FREE_BLOCK);
                    indirectBlock[i] = 0;
                    parentInode->size -= BLOCK_SIZE;
                    DevWriteBlock(block_number, indirectBlock);
                }

                //dislink indirectBLOCK
                int numIndirectDE = 0;
                for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
                    if (indirectBlock[i] != 0) {
                        numIndirectDE++;
                    }
                }
                if (numIndirectDE == 0) {
                    ResetInodeBitmap(parentInode->indirBlockPtr);
                    updateFileSysInfo(FREE_INODE);
                    parentInode->indirBlockPtr = 0;
                    PutInode(parentInodeNum, parentInode);
                }

                PutInode(parentInodeNum, parentInode);
                PutInode(*current_inode_num, pInode);
                //dislink with parent

                free(pInode);
                free(current_inode_num);
                free(parentBlockNum);
                free(fInode);
                free(pBlock);
                free(parentInode);
                return 0;
            }
        }
    }


}


int MakeDir(const char *szDirName) {

    int *current_inode_num = (int *) malloc(sizeof(int));
    *current_inode_num = 0;

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
        goDownDir(arr[oh_array_index], pInode, current_inode_num, parentBlockNum);// 이게 돌아주면 될것 같다
    }
    int parentInodeNum = *current_inode_num;
    int i, j;
    int block_number;//이거 왜 1이 들어가는거야 시발
    int newBlockNum = -1;
    DirEntry dir_entry[NUM_OF_DIRENT_PER_BLOCK];
    memset(dir_entry, 0, BLOCK_SIZE);



    //travel direct pointers
    for (i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {

        block_number = pInode->dirBlockPtr[i];
        if (block_number == 0) { //add new direct pointer and new entry

            int newBlockNum = GetFreeBlockNum();
            updateFileSysInfo(ALOCATE_BLOCK);
            SetBlockBitmap(newBlockNum);

            //init new Entry Block
            DirEntry newDirEntry[NUM_OF_DIRENT_PER_BLOCK];
            memset(newDirEntry, 0, BLOCK_SIZE);

            //link with parent Block
            pInode->dirBlockPtr[i] = newBlockNum; //ex) 19

            DevWriteBlock(newBlockNum, newDirEntry);

            pInode->dirBlockPtr[i] = newBlockNum;
            pInode->size += BLOCK_SIZE;
            block_number = pInode->dirBlockPtr[i];
            PutInode(parentInodeNum, pInode);
        }
        DevReadBlock(block_number, (char *) dir_entry);

        //add new dir
        for (j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) { //4
            if (strcmp(dir_entry[j].name, "") == 0) {
                for (int i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++) {
                    if (strcmp(dir_entry[i].name, "") == 0) {

                        int newInodeNum;
                        newInodeNum = GetFreeInodeNum();
                        updateFileSysInfo(ALOCATE_INODE);
                        SetInodeBitmap(newInodeNum);
                        Inode *newInode;
                        newInode = (Inode *) malloc(BLOCK_SIZE);
                        newInode->type = FILE_TYPE_DIR;
                        newInode->size = 0;
                        PutInode(newInodeNum, newInode);

                        strcpy(dir_entry[i].name, arr[arr_index - 1]);
                        dir_entry[i].inodeNum = newInodeNum;
                        DevWriteBlock(block_number, (char *) dir_entry);

                        newBlockNum = GetFreeBlockNum();
                        SetBlockBitmap(newBlockNum);
                        updateFileSysInfo(ALOCATE_BLOCK);
                        DirEntry newDE[NUM_OF_DIRENT_PER_BLOCK];
                        memset(newDE, 0, BLOCK_SIZE);
                        strcpy(newDE[0].name, ".");
                        newDE[0].inodeNum = newInodeNum;
                        strcpy(newDE[1].name, "..");
                        newDE[1].inodeNum = parentInodeNum;
                        DevWriteBlock(newBlockNum, (char *) newDE);
                        DevWriteBlock(block_number, (char *) dir_entry);

                        newInode->dirBlockPtr[0] = newBlockNum;
                        PutInode(newInodeNum, newInode);
                        free(newInode);
                        return 0;
                    }
                }
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
        block_number = newBlockNum;
        int indirectBlock[BLOCK_SIZE / sizeof(int)];
        memset(indirectBlock, 0, BLOCK_SIZE);
        pInode->indirBlockPtr = indirectBlockNum;
        DevWriteBlock(block_number, (char *) indirectBlock);
        PutInode(parentInodeNum, pInode);
    }

    int indirectBlock[BLOCK_SIZE / sizeof(int)];
    DevReadBlock(indirectBlockNum, (char *) indirectBlock);

    for (i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
        block_number = indirectBlock[i];
        DirEntry dir_entry[NUM_OF_DIRENT_PER_BLOCK];

        if (block_number == 0) { //add new block for indirectBlock

            newBlockNum = GetFreeBlockNum();
            SetBlockBitmap(newBlockNum);
            updateFileSysInfo(ALOCATE_BLOCK);
            pInode->size += BLOCK_SIZE;
            indirectBlock[i] = newBlockNum;
            DirEntry *newDirEntry = (DirEntry *) malloc(BLOCK_SIZE);
            memset(newDirEntry, 0, BLOCK_SIZE);
            DevWriteBlock(newBlockNum, (char *) newDirEntry);
            DevWriteBlock(indirectBlockNum, (char *) indirectBlock);
            free(newDirEntry);
        }
        block_number = indirectBlock[i];
        DevReadBlock(block_number, dir_entry);

        for (j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (strcmp(dir_entry[j].name, "") == 0) { //get empty dirent number
                for (int i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++) {
                    if (strcmp(dir_entry[i].name, "") == 0) {

                        int newInodeNum;
                        newInodeNum = GetFreeInodeNum();
                        updateFileSysInfo(ALOCATE_INODE);
                        SetInodeBitmap(newInodeNum);
                        Inode *newInode;
                        newInode = (Inode *) malloc(BLOCK_SIZE);
                        newInode->type = FILE_TYPE_DIR;
                        newInode->size = 0;
                        PutInode(newInodeNum, newInode);

                        strcpy(dir_entry[i].name, arr[arr_index - 1]);
                        dir_entry[i].inodeNum = newInodeNum;
                        DevWriteBlock(block_number, (char *) dir_entry);

                        newBlockNum = GetFreeBlockNum();
                        SetBlockBitmap(newBlockNum);
                        updateFileSysInfo(ALOCATE_BLOCK);
                        DirEntry newDE[NUM_OF_DIRENT_PER_BLOCK];
                        memset(newDE, 0, BLOCK_SIZE);
                        strcpy(newDE[0].name, ".");
                        newDE[0].inodeNum = newInodeNum;
                        strcpy(newDE[1].name, "..");
                        newDE[1].inodeNum = parentInodeNum;
                        DevWriteBlock(newBlockNum, (char *) newDE);
                        DevWriteBlock(block_number, (char *) dir_entry);

                        newInode->dirBlockPtr[0] = newBlockNum;
                        PutInode(newInodeNum, newInode);
                        DevWriteBlock(indirectBlockNum, (char *) indirectBlock);
                        return 0;
                    }
                }

            }
        }
    }
    return -1;

}


int RemoveDir(const char *szDirName) {
    int *current_inode_num = (int *) malloc(sizeof(int));
    *current_inode_num = 0;

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
        goDownDir(arr[oh_array_index], pInode, current_inode_num, parentBlockNum);// 이게 돌아주면 될것 같다
    }
    int parentInodeNum = *current_inode_num;
    goDownDir(arr[oh_array_index], pInode, current_inode_num, parentBlockNum);// 이게 돌아주면 될것 같다
    if (*current_inode_num == -1) //no such directory
        return -1;

    char *pBlock = (char *) malloc(BLOCK_SIZE);

    ResetBlockBitmap(pInode->dirBlockPtr[0]);
    ResetInodeBitmap(*current_inode_num);
    updateFileSysInfo(FREE_INODE);
    updateFileSysInfo(FREE_BLOCK);
    Inode *parentInode = (Inode *) malloc(BLOCK_SIZE);
    GetInode(parentInodeNum, parentInode);
    //disLinkWithParent(parentInode, *current_inode_num, *current_inode_num);

    int i, j;
    int block_number;

    //direct DE
    DirEntry dir_entry[NUM_OF_DIRENT_PER_BLOCK];
    for (i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {
        block_number = parentInode->dirBlockPtr[i];
        DevReadBlock(block_number, dir_entry);
        for (j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (dir_entry[j].inodeNum == *current_inode_num) {
                dir_entry[j].inodeNum = 0;
                memset(dir_entry[j].name, 0, sizeof(dir_entry[j].name));
                //strcpy(dir_entry[j].name,"");
                DevWriteBlock(block_number, (char *) dir_entry);

                //if number of dir_entry equal 0, also dislink DE with parent;
                int numOfEmptyBlockInDE = 0;
                for (int i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++) {
                    if (dir_entry[i].inodeNum == 0)
                        numOfEmptyBlockInDE++;
                }
                if (numOfEmptyBlockInDE == 4) {
                    ResetBlockBitmap(block_number);
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
    int inode_block_num;
    block_number = parentInode->indirBlockPtr;
    DevReadBlock(block_number, (char *) indirectBlock);
    for (i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
        inode_block_num = indirectBlock[i];
        DevReadBlock(inode_block_num, (char *) dir_entry);
        for (j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (dir_entry[j].inodeNum == *current_inode_num) {
                dir_entry[j].inodeNum = 0;
                updateFileSysInfo(FREE_INODE);
                ResetInodeBitmap(*current_inode_num);
                memset(dir_entry[j].name, 0, sizeof(dir_entry[j].name));
                //strcpy(dir_entry[j].name,"");
                DevWriteBlock(inode_block_num, dir_entry);

                int numOfEmptyBlockInDE = 0;
                for (int i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++) {
                    if (dir_entry[i].inodeNum == 0)
                        numOfEmptyBlockInDE++;
                }
                //dir_entry has no child, free block
                if (numOfEmptyBlockInDE == 4) {
                    ResetBlockBitmap(inode_block_num);
                    updateFileSysInfo(FREE_BLOCK);
                    indirectBlock[i] = 0;
                    parentInode->size -= BLOCK_SIZE;
                    DevWriteBlock(block_number, (char *) indirectBlock);
                }

                //dislink indirectBLOCK
                int numIndirectDE = 0;
                for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
                    if (indirectBlock[i] != 0) {
                        numIndirectDE++;
                    }
                }
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
    free(current_inode_num);
    free(parentBlockNum);
    free(pBlock);
    free(parentInode);
    return 0;

}

int EnumerateBlocks(DirEntry *dir_entry, DirEntryInfo *pDirEntry, int dirEntrys, int count) {
    int i;
    Inode *tempInode = (Inode *) malloc(sizeof(Inode));

    for (i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++) {
        if (count >= dirEntrys)
            break;
        if (strcmp(dir_entry[i].name, "") == 0)
            continue;
        pDirEntry[count].inodeNum = dir_entry[i].inodeNum;
        strcpy(pDirEntry[count].name, dir_entry[i].name);
        GetInode(dir_entry[i].inodeNum, tempInode);
        pDirEntry[count].type = tempInode->type;
        count++;

    }
    free(tempInode);
    return count;
}

int EnumerateDirStatus(const char *szDirName, DirEntryInfo *pDirEntry, int dirEntrys) {
    Inode *pInode = (Inode *) malloc(sizeof(Inode));
    int *current_inode_num = (int *) malloc(sizeof(int));
    *current_inode_num = 0;
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
         goDownDir(arr[oh_array_index], pInode, current_inode_num, parentBlockNum);// 이게 돌아주면 될것 같다
    }
    int parentInodeNum = *current_inode_num;
     goDownDir(arr[oh_array_index], pInode, current_inode_num, parentBlockNum);// 이게 돌아주면 될것 같다
    if (parentInodeNum == -1 || *current_inode_num == -1) {
        free(pInode);
        free(current_inode_num);
        free(parentBlockNum);
        return -1;
    }

    Inode *parentInode = (Inode *) malloc(sizeof(Inode));
    GetInode(parentInodeNum, parentInode);
    int i, count = 0;
    int block_number;

    //direct
    DirEntry dir_entry[NUM_OF_DIRENT_PER_BLOCK];
    for (i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {
        block_number = pInode->dirBlockPtr[i];
        if (block_number == 0)
            continue;
        DevReadBlock(block_number, (char *) dir_entry);
        Inode *tempInode = (Inode *) malloc(sizeof(Inode));
        count = EnumerateBlocks(dir_entry, pDirEntry, dirEntrys, count);

        free(tempInode);
        if (count >= dirEntrys) {

            free(pInode);
            free(current_inode_num);
            free(parentBlockNum);
            free(parentInode);
            return count;
        }
    }

    //indirect
    int inode_block_num = pInode->indirBlockPtr;
    if (inode_block_num != 0) {
        int indirectBlock[BLOCK_SIZE / sizeof(int)];
        DevReadBlock(inode_block_num, (char *) indirectBlock);
        for (i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
            block_number = indirectBlock[i];
            if (block_number == 0)
                continue;
            DevReadBlock(block_number, dir_entry);
            count = EnumerateBlocks(dir_entry, pDirEntry, dirEntrys, count);
            if (count >= dirEntrys) {
                free(pInode);
                free(current_inode_num);
                free(parentBlockNum);
                free(parentInode);
                return count;
            }
        }
    }
    free(pInode);
    free(current_inode_num);
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