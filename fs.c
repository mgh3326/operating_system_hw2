//-fs.c: 과제1에서 구현한 코드를 이 파일에 넣고, 빈 함수를 구현하면 됩니다.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "disk.h"
#include "fs.h"
#include <math.h>


FileDescTable *pFileDescTable = NULL;

int goDownDir(char *find_filename, Inode *curInode) {//이거는 많이 쓸거 같다

    DirEntry directory_entry[NUM_OF_DIRENT_PER_BLOCK];
    for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {
        DevReadBlock(curInode->dirBlockPtr[i], (char *) directory_entry);
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (strcmp(directory_entry[j].name, find_filename) == 0) {
                GetInode(directory_entry[j].inodeNum, curInode);
                return directory_entry[j].inodeNum;
            }
        }
    }

    int indirectBlock[BLOCK_SIZE / sizeof(int)];
    DevReadBlock(curInode->indirBlockPtr, (char *) indirectBlock);
    for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
        DevReadBlock(indirectBlock[i], directory_entry);
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (strcmp(directory_entry[j].name, find_filename) == 0) {
                GetInode(directory_entry[j].inodeNum, curInode);
                return directory_entry[j].inodeNum;
//                return *curInodeNum;
            }
        }
    }
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
        *curInodeNum = goDownDir(arr[oh_array_index], pInode);// 이게 돌아주면 될것 같다
    }
    int parentInodeNum = *curInodeNum;
    if (flag == OPEN_FLAG_READWRITE) {
        *curInodeNum = goDownDir(arr[oh_array_index], pInode);// 이게 돌아주면 될것 같다
        for (int i = 0; i < MAX_FD_ENTRY_LEN; i++) {
            if (pFileDescTable->file[i].bUsed == 0) {
                pFileDescTable->file[i].bUsed = 1;
                pFileDescTable->file[i].inodeNum = *curInodeNum;
                pFileDescTable->file[i].fileOffset = 0;
                return i;
            }
        }
    } else if (flag == OPEN_FLAG_CREATE) {
//        addFileDir(arr[arr_index], parentInodeNum);
        //int newInodeNum = addFileDir(arr[arr_index - 1], parentInodeNum);



        int newBlockNum;
        Inode *parentInode = (Inode *) malloc(BLOCK_SIZE);
        GetInode(parentInodeNum, parentInode);
        DirEntry directory_entry[NUM_OF_DIRENT_PER_BLOCK];


        //travel direct pointers
        for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {

            int bnum = parentInode->dirBlockPtr[i];
            if (parentInode->dirBlockPtr[i] == 0) { //add new direct pointer and new entry

                newBlockNum = GetFreeBlockNum();
                DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
                pFileSysInfo->numAllocBlocks++;
                pFileSysInfo->numFreeBlocks--;
                DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);

                SetBlockBitmap(newBlockNum);

                //init new Entry Block
                DirEntry newDirEntry[NUM_OF_DIRENT_PER_BLOCK];
                memset(newDirEntry, 0, BLOCK_SIZE);

                //link with parent Block
                parentInode->dirBlockPtr[i] = newBlockNum; //ex) 19

                DevWriteBlock(newBlockNum, newDirEntry);


                parentInode->dirBlockPtr[i] = newBlockNum;
                parentInode->size += BLOCK_SIZE;
                bnum = parentInode->dirBlockPtr[i];
                PutInode(parentInodeNum, parentInode);
            }
            DevReadBlock(parentInode->dirBlockPtr[i], directory_entry);

            //add new dir
            for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) { //4
                if (strcmp(directory_entry[j].name, "") == 0) {


                    for (int i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++) {
                        if (strcmp(directory_entry[i].name, "") == 0) {

                            //save child directory_entry
                            int newInodeNum = GetFreeInodeNum();

                            DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
                            pFileSysInfo->numAllocInodes++;
                            DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                            SetInodeBitmap(newInodeNum);
                            Inode *newInode = (Inode *) malloc(BLOCK_SIZE);
                            memset(newInode, 0, sizeof(newInode));//memory init !!!
                            newInode->type = FILE_TYPE_FILE;
                            newInode->size = 0;

                            PutInode(newInodeNum, newInode);

                            //save parent directory_entry
                            strcpy(directory_entry[i].name, arr[arr_index - 1]);
                            directory_entry[i].inodeNum = newInodeNum;
                            DevWriteBlock(bnum, (char *) directory_entry);

                            free(newInode);
                            free(parentInode);

                            for (int i = 0; i < MAX_FD_ENTRY_LEN; i++) {
                                if (pFileDescTable->file[i].bUsed == 0) {
                                    pFileDescTable->file[i].bUsed = 1;
                                    pFileDescTable->file[i].inodeNum = newInodeNum;
                                    pFileDescTable->file[i].fileOffset = 0;
                                    free(pInode);
                                    free(curInodeNum);
                                    free(parentBlockNum);
                                    return i;
                                }
                            }

                        }
                    }


//                return newInodeNum;
                }
            }
        }

        //--------indirect------

        //travel indirect block
        int indirectBlockNum = parentInode->indirBlockPtr;
        if (indirectBlockNum == 0) { //does't allocate indirect Block yet
            //add new indirect Block
            newBlockNum = GetFreeBlockNum();


            DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
            pFileSysInfo->numAllocBlocks++;
            pFileSysInfo->numFreeBlocks--;
            DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
            SetBlockBitmap(newBlockNum);
            indirectBlockNum = newBlockNum;

            int indirectBlock[BLOCK_SIZE / sizeof(int)];
            memset(indirectBlock, 0, BLOCK_SIZE);
            parentInode->indirBlockPtr = indirectBlockNum;
            DevWriteBlock(newBlockNum, indirectBlock);
            PutInode(parentInodeNum, parentInode);
        }

        int indirectBlock[BLOCK_SIZE / sizeof(int)];
        DevReadBlock(indirectBlockNum, indirectBlock);

        for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++) {

            DirEntry directory_entry[NUM_OF_DIRENT_PER_BLOCK];

            if (indirectBlock[i] == 0) { //add new block for indirectBlock
                int newBlockNum = GetFreeBlockNum();
                SetBlockBitmap(newBlockNum);
                DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
                pFileSysInfo->numAllocBlocks++;
                pFileSysInfo->numFreeBlocks--;
                DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                parentInode->size += BLOCK_SIZE;
                indirectBlock[i] = newBlockNum;
                DirEntry *newDirEntry = (DirEntry *) malloc(BLOCK_SIZE);
                memset(newDirEntry, 0, BLOCK_SIZE);
                DevWriteBlock(newBlockNum, newDirEntry);
                DevWriteBlock(indirectBlockNum, indirectBlock);
                free(newDirEntry);
            };
            DevReadBlock(indirectBlock[i], directory_entry);

            for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
                if (strcmp(directory_entry[j].name, "") == 0) { //get empty dirent number


                    for (int i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++) {
                        if (strcmp(directory_entry[i].name, "") == 0) {

                            //save child directory_entry
                            int newInodeNum = GetFreeInodeNum();
                            DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
                            pFileSysInfo->numAllocInodes++;
                            DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                            SetInodeBitmap(newInodeNum);
                            Inode *newInode = (Inode *) malloc(BLOCK_SIZE);
                            memset(newInode, 0, sizeof(newInode));//memory init !!!
                            newInode->type = FILE_TYPE_FILE;
                            newInode->size = 0;

                            PutInode(newInodeNum, newInode);

                            //save parent directory_entry
                            strcpy(directory_entry[i].name, arr[arr_index - 1]);
                            directory_entry[i].inodeNum = newInodeNum;
                            DevWriteBlock(indirectBlock[i], directory_entry);

                            free(newInode);
                            DevWriteBlock(indirectBlockNum, indirectBlock);
                            free(parentInode);
                            for (int i = 0; i < MAX_FD_ENTRY_LEN; i++) {
                                if (pFileDescTable->file[i].bUsed == 0) {
                                    pFileDescTable->file[i].bUsed = 1;
                                    pFileDescTable->file[i].inodeNum = newInodeNum;
                                    pFileDescTable->file[i].fileOffset = 0;
                                    free(pInode);
                                    free(curInodeNum);
                                    free(parentBlockNum);
                                    return i;
                                }
                            }

                        }
                    }

//                return newInodeNum;
                }
            }
        }
        free(parentInode);
        return -1;





        //int returnValue = initNewFile(newInodeNum);

        return -1;

    }
//파일이 있다고 가정하에 하는거여서 문제가 발생하는듯 하다.

}

void addBlockInInode(Inode *file_inode, int inodeNum, int writeBlockNum) {

    for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {
        if (file_inode->dirBlockPtr[i] == 0) {
            file_inode->dirBlockPtr[i] = writeBlockNum;
            PutInode(inodeNum, file_inode);
            return;
        }
    }

    if (file_inode->indirBlockPtr == 0) { //does't allocate indirect Block yet
        //add new indirect Block

        DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
        pFileSysInfo->numAllocBlocks++;
        pFileSysInfo->numFreeBlocks--;
        DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
        file_inode->indirBlockPtr = GetFreeBlockNum();

        SetBlockBitmap(GetFreeBlockNum());
        int indirectBlock[BLOCK_SIZE / sizeof(int)];
        memset(indirectBlock, 0, BLOCK_SIZE);

        DevWriteBlock(file_inode->indirBlockPtr, (char *) indirectBlock);
        PutInode(inodeNum, file_inode);
    }
    int indirectBlock[BLOCK_SIZE / sizeof(int)];
    DevReadBlock(file_inode->indirBlockPtr, (char *) indirectBlock);
    //link with indirect
    for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++) {


        if (indirectBlock[i] == 0) { //add new block for indirectBlock
            char *newFileBlock = (char *) malloc(BLOCK_SIZE);
            indirectBlock[i] = writeBlockNum;
            memset(newFileBlock, 0, BLOCK_SIZE);
            DevWriteBlock(writeBlockNum, newFileBlock);
            DevWriteBlock(file_inode->indirBlockPtr, (char *) indirectBlock);
            free(newFileBlock);
            return;
        }
    }
}


int WriteFile(int fileDesc, char *pBuffer, int length) {

    Inode *file_inode = (Inode *) malloc(sizeof(Inode));

    GetInode(pFileDescTable->file[fileDesc].inodeNum, file_inode);

    char *writeBlock = (char *) malloc(BLOCK_SIZE);
    memset(writeBlock, 0, sizeof(BLOCK_SIZE));
    int writeBlockNum = 0;
    int flow;
    int totalWriteSize = 0;
    int saveFileBlockNum;

    while (totalWriteSize < length) {
        flow = pFileDescTable->file[fileDesc].fileOffset + totalWriteSize;
        //if (flow % BLOCK_SIZE == 0) {//no data or full block
        if (file_inode->size % BLOCK_SIZE == 0) { //no data or full block
            writeBlockNum = GetFreeBlockNum();
            DevWriteBlock(writeBlockNum, writeBlock);
            DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
            pFileSysInfo->numAllocBlocks++;
            pFileSysInfo->numFreeBlocks--;
            DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
            SetBlockBitmap(writeBlockNum);
            addBlockInInode(file_inode, pFileDescTable->file[fileDesc].inodeNum, writeBlockNum);


        }
        if (flow / BLOCK_SIZE == 0)
            saveFileBlockNum = file_inode->dirBlockPtr[0];
        else if (flow / BLOCK_SIZE == 1)
            saveFileBlockNum = file_inode->dirBlockPtr[1];
        else {
            int indirectBlock[BLOCK_SIZE / sizeof(int)];
            DevReadBlock(file_inode->indirBlockPtr, (char *) indirectBlock);
            saveFileBlockNum = indirectBlock[(flow / BLOCK_SIZE) - 2];
        }
        char saveBlock[BLOCK_SIZE];
        DevReadBlock(saveFileBlockNum, saveBlock);

        int availableByte = ((flow / BLOCK_SIZE + 1) * BLOCK_SIZE) - flow;


        int writeStartIndex = BLOCK_SIZE - availableByte;
        for (int i = 0; i < availableByte; i++) {
            saveBlock[writeStartIndex] = pBuffer[totalWriteSize];
            writeStartIndex++;
            totalWriteSize++;
            if (totalWriteSize >= length || totalWriteSize >= (int) strlen(pBuffer)) {
                DevWriteBlock(saveFileBlockNum, saveBlock);
                pFileDescTable->file[fileDesc].fileOffset += totalWriteSize;
                file_inode->size += totalWriteSize;
                PutInode(pFileDescTable->file[fileDesc].inodeNum, file_inode);
                free(file_inode);
                free(writeBlock);
                return totalWriteSize;

            }
        }
        DevWriteBlock(saveFileBlockNum, saveBlock);

    }

}

int ReadFile(int fileDesc, char *pBuffer, int length) {
    Inode *file_inode;
    file_inode = (Inode *) malloc(sizeof(Inode));
    GetInode(pFileDescTable->file[fileDesc].inodeNum, file_inode);

    char *readBlock = (char *) malloc(BLOCK_SIZE);
    int readBlockNum;
    int flow;
    int totalReadSize = 0;


    while (totalReadSize < length) {
        flow = pFileDescTable->file[fileDesc].fileOffset + totalReadSize;
        if (flow >= file_inode->size)
            break;

        if (flow / BLOCK_SIZE == 0)
            readBlockNum = file_inode->dirBlockPtr[0];
        else if (flow / BLOCK_SIZE == 1)
            readBlockNum = file_inode->dirBlockPtr[1];
        else {
            int indirectBlock[BLOCK_SIZE / sizeof(int)];
            DevReadBlock(file_inode->indirBlockPtr, (char *) indirectBlock);
            readBlockNum = indirectBlock[(flow / BLOCK_SIZE) - 2];
        }
        DevReadBlock(readBlockNum, readBlock);

        int availableByte = ((flow / BLOCK_SIZE + 1) * BLOCK_SIZE) - flow;
        int readStartIndex = BLOCK_SIZE - availableByte;

        for (int i = 0; i < availableByte; i++) {


            pBuffer[totalReadSize] = readBlock[readStartIndex];
            readStartIndex++;
            totalReadSize++;
            if (totalReadSize >= length) {
                pFileDescTable->file[fileDesc].fileOffset += totalReadSize;
                free(file_inode);
                free(readBlock);
                return totalReadSize;
            }
        }
    }
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
        *curInodeNum = goDownDir(arr[oh_array_index], pInode);// 이게 돌아주면 될것 같다
    }
    int parentInodeNum = *curInodeNum;
    *curInodeNum = goDownDir(arr[oh_array_index], pInode);// 이게 돌아주면 될것 같다


    if (*curInodeNum == -1) {
        free(pInode);
        free(curInodeNum);
        free(parentBlockNum);
        return -1;
    }

    //int fileInodeNum = getFileInodeNum(pInode,find_filename);

    for (int i = 0; i < MAX_FD_ENTRY_LEN; i++) {
        if (pFileDescTable->file[i].inodeNum == *curInodeNum) {
            free(pInode);
            free(curInodeNum);
            free(parentBlockNum);
            return -1;
        }

    }


    Inode *file_inode = (Inode *) malloc(sizeof(Inode));
    GetInode(*curInodeNum, file_inode);

    char *pBlock = (char *) malloc(BLOCK_SIZE);
    for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {

        if (file_inode->dirBlockPtr[i] == 0)
            continue;
        DevReadBlock(file_inode->dirBlockPtr[i], pBlock);
        memset(pBlock, 0, BLOCK_SIZE);
        DevWriteBlock(file_inode->dirBlockPtr[i], pBlock);
        ResetBlockBitmap(file_inode->dirBlockPtr[i]);
        DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
        pFileSysInfo->numAllocBlocks--;
        pFileSysInfo->numFreeBlocks++;
        DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
        file_inode->dirBlockPtr[i] = 0;
    }

    if (file_inode->indirBlockPtr != 0) {

        int indirectBlock[BLOCK_SIZE / sizeof(int)];
        DevReadBlock(file_inode->indirBlockPtr, (char *) indirectBlock);
        for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++) {

            if (indirectBlock[i] == 0)
                continue;
            DevReadBlock(indirectBlock[i], pBlock);
            memset(pBlock, 0, BLOCK_SIZE);
            DevWriteBlock(indirectBlock[i], pBlock);
            ResetBlockBitmap(indirectBlock[i]);
            DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
            pFileSysInfo->numAllocBlocks--;
            pFileSysInfo->numFreeBlocks++;
            DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
        }
        DevReadBlock(file_inode->indirBlockPtr, pBlock);
        memset(pBlock, 0, BLOCK_SIZE);
        DevWriteBlock(file_inode->indirBlockPtr, pBlock);
        ResetBlockBitmap(file_inode->indirBlockPtr);
        DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
        pFileSysInfo->numAllocBlocks--;
        pFileSysInfo->numFreeBlocks++;
        DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
        file_inode->indirBlockPtr = 0;
    }
    //free inode,
    ResetInodeBitmap(*curInodeNum);
    DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
    pFileSysInfo->numAllocInodes--;
    DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
    file_inode->size = 0;
    file_inode->type = 0;

    Inode *parentInode = (Inode *) malloc(BLOCK_SIZE);
    GetInode(parentInodeNum, parentInode);

    DirEntry directory_entry[NUM_OF_DIRENT_PER_BLOCK];
    for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {

        DevReadBlock(parentInode->dirBlockPtr[i], directory_entry);
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (directory_entry[j].inodeNum == *curInodeNum) {
                directory_entry[j].inodeNum = 0;
                memset(directory_entry[j].name, 0, sizeof(directory_entry[j].name));
                //strcpy(directory_entry[j].name,"");
                DevWriteBlock(parentInode->dirBlockPtr[i], directory_entry);

                //if number of directory_entry equal 0, also dislink directory_entry with parent;
                int numOfEmptyBlockInDE = 0;
                for (int i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++) {
                    if (directory_entry[i].inodeNum == 0)
                        numOfEmptyBlockInDE++;
                }
                if (numOfEmptyBlockInDE == 4) {
                    ResetBlockBitmap(parentInode->dirBlockPtr[i]);
                    DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
                    pFileSysInfo->numAllocBlocks--;
                    pFileSysInfo->numFreeBlocks++;
                    DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);

                    parentInode->dirBlockPtr[i] = 0;
                    parentInode->size -= BLOCK_SIZE;
                    PutInode(parentInodeNum, parentInode);
                }


                PutInode(parentInodeNum, parentInode);
                PutInode(*curInodeNum, pInode);
                //dislink with parent

                free(pInode);
                free(curInodeNum);
                free(parentBlockNum);
                free(file_inode);
                free(pBlock);
                free(parentInode);
                return 0;
            }
        }
    }
    int indirectBlock[BLOCK_SIZE / sizeof(int)];
    DevReadBlock(parentInode->indirBlockPtr, indirectBlock);
    for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++) {

        DevReadBlock(indirectBlock[i], directory_entry);
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (directory_entry[j].inodeNum == *curInodeNum) {
                directory_entry[j].inodeNum = 0;


                DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
                pFileSysInfo->numAllocInodes--;
                DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);

                ResetInodeBitmap(*curInodeNum);
                memset(directory_entry[j].name, 0, sizeof(directory_entry[j].name));
                //strcpy(directory_entry[j].name,"");
                DevWriteBlock(indirectBlock[i], directory_entry);

                int numOfEmptyBlockInDE = 0;
                for (int i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++) {
                    if (directory_entry[i].inodeNum == 0)
                        numOfEmptyBlockInDE++;
                }
                //directory_entry has no child, free block
                if (numOfEmptyBlockInDE == 4) {
                    ResetBlockBitmap(indirectBlock[i]);
                    DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
                    pFileSysInfo->numAllocBlocks--;
                    pFileSysInfo->numFreeBlocks++;
                    DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                    indirectBlock[i] = 0;
                    parentInode->size -= BLOCK_SIZE;
                    DevWriteBlock(parentInode->indirBlockPtr, indirectBlock);
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

                    DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
                    pFileSysInfo->numAllocInodes--;
                    DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                    parentInode->indirBlockPtr = 0;
                    PutInode(parentInodeNum, parentInode);

                }

                PutInode(parentInodeNum, parentInode);
                PutInode(*curInodeNum, pInode);
                //dislink with parent

                free(pInode);
                free(curInodeNum);
                free(parentBlockNum);
                free(file_inode);
                free(pBlock);
                free(parentInode);
                return 0;
            }
        }
    }


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
        *curInodeNum = goDownDir(arr[oh_array_index], pInode);// 이게 돌아주면 될것 같다
    }
    int parentInodeNum = *curInodeNum;
    int i, j;
    int bnum;//이거 왜 1이 들어가는거야 시발
    int newBlockNum = -1;
    DirEntry directory_entry[NUM_OF_DIRENT_PER_BLOCK];
    memset(directory_entry, 0, BLOCK_SIZE);



    //travel direct pointers
    for (i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {

        bnum = pInode->dirBlockPtr[i];
        if (bnum == 0) { //add new direct pointer and new entry

            int newBlockNum = GetFreeBlockNum();
            DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
            pFileSysInfo->numAllocBlocks++;
            pFileSysInfo->numFreeBlocks--;
            DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
            SetBlockBitmap(newBlockNum);

            //init new Entry Block
            DirEntry newDirEntry[NUM_OF_DIRENT_PER_BLOCK];
            memset(newDirEntry, 0, BLOCK_SIZE);

            //link with parent Block
            pInode->dirBlockPtr[i] = newBlockNum; //ex) 19

            DevWriteBlock(newBlockNum, newDirEntry);

            pInode->dirBlockPtr[i] = newBlockNum;
            pInode->size += BLOCK_SIZE;
            bnum = pInode->dirBlockPtr[i];
            PutInode(parentInodeNum, pInode);
        }
        DevReadBlock(bnum, directory_entry);

        //add new dir
        for (j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) { //4
            if (strcmp(directory_entry[j].name, "") == 0) {
//                newBlockNum = addNewDirBlock(arr[arr_index - 1], parentInodeNum, bnum, directory_entry);

                for (int i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++) {
                    if (strcmp(directory_entry[i].name, "") == 0) {

                        int newInodeNum = GetFreeInodeNum();
                        DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
                        pFileSysInfo->numAllocInodes++;
                        DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                        SetInodeBitmap(newInodeNum);
                        Inode *newInode = (Inode *) malloc(BLOCK_SIZE);
                        newInode->type = FILE_TYPE_DIR;
                        newInode->size = 0;
                        PutInode(newInodeNum, newInode);

                        strcpy(directory_entry[i].name, arr[arr_index - 1]);
                        directory_entry[i].inodeNum = newInodeNum;
                        DevWriteBlock(bnum, (char *) directory_entry);

                        int newBlockNum = GetFreeBlockNum();
                        SetBlockBitmap(newBlockNum);
                        DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
                        pFileSysInfo->numAllocBlocks++;
                        pFileSysInfo->numFreeBlocks--;
                        DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                        DirEntry newDE[NUM_OF_DIRENT_PER_BLOCK];
                        memset(newDE, 0, BLOCK_SIZE);
                        strcpy(newDE[0].name, ".");
                        newDE[0].inodeNum = newInodeNum;
                        strcpy(newDE[1].name, "..");
                        newDE[1].inodeNum = parentInodeNum;
                        DevWriteBlock(newBlockNum, (char *) newDE);
                        DevWriteBlock(bnum, (char *) directory_entry);

                        newInode->dirBlockPtr[0] = newBlockNum;
                        PutInode(newInodeNum, newInode);
                        free(newInode);
                        return 0;
                    }
                }
                return -1;


            }
        }
    }

    //--------indirect------

    //travel indirect block
    int indirectBlockNum = pInode->indirBlockPtr;
    if (indirectBlockNum == 0) { //does't allocate indirect Block yet
        //add new indirect Block
        newBlockNum = GetFreeBlockNum();
        DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
        pFileSysInfo->numAllocBlocks++;
        pFileSysInfo->numFreeBlocks--;
        DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
        SetBlockBitmap(newBlockNum);
        indirectBlockNum = newBlockNum;
        bnum = newBlockNum;
        int indirectBlock[BLOCK_SIZE / sizeof(int)];
        memset(indirectBlock, 0, BLOCK_SIZE);
        pInode->indirBlockPtr = indirectBlockNum;
        DevWriteBlock(bnum, indirectBlock);
        PutInode(parentInodeNum, pInode);
    }

    int indirectBlock[BLOCK_SIZE / sizeof(int)];
    DevReadBlock(indirectBlockNum, indirectBlock);

    for (i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
        bnum = indirectBlock[i];
        DirEntry directory_entry[NUM_OF_DIRENT_PER_BLOCK];

        if (bnum == 0) { //add new block for indirectBlock
            int newBlockNum = GetFreeBlockNum();
            SetBlockBitmap(newBlockNum);
            DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
            pFileSysInfo->numAllocBlocks++;
            pFileSysInfo->numFreeBlocks--;
            DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
            pInode->size += BLOCK_SIZE;
            indirectBlock[i] = newBlockNum;
            DirEntry *newDirEntry = (DirEntry *) malloc(BLOCK_SIZE);
            memset(newDirEntry, 0, BLOCK_SIZE);
            DevWriteBlock(newBlockNum, newDirEntry);
            DevWriteBlock(indirectBlockNum, indirectBlock);
            free(newDirEntry);
        }
        bnum = indirectBlock[i];
        DevReadBlock(indirectBlock[i], directory_entry);

        for (j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (strcmp(directory_entry[j].name, "") == 0) { //get empty dirent number
//                newBlockNum = addNewDirBlock(arr[arr_index - 1], parentInodeNum, bnum, directory_entry);

                for (int i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++) {
                    if (strcmp(directory_entry[i].name, "") == 0) {

                        int newInodeNum = GetFreeInodeNum();
                        DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
                        pFileSysInfo->numAllocInodes++;
                        DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                        SetInodeBitmap(newInodeNum);
                        Inode *newInode = (Inode *) malloc(BLOCK_SIZE);
                        newInode->type = FILE_TYPE_DIR;
                        newInode->size = 0;
                        PutInode(newInodeNum, newInode);

                        strcpy(directory_entry[i].name, arr[arr_index - 1]);
                        directory_entry[i].inodeNum = newInodeNum;
                        DevWriteBlock(bnum, (char *) directory_entry);

                        int newBlockNum = GetFreeBlockNum();
                        SetBlockBitmap(newBlockNum);
                        DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
                        pFileSysInfo->numAllocBlocks++;
                        pFileSysInfo->numFreeBlocks--;
                        DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                        DirEntry newDE[NUM_OF_DIRENT_PER_BLOCK];
                        memset(newDE, 0, BLOCK_SIZE);
                        strcpy(newDE[0].name, ".");
                        newDE[0].inodeNum = newInodeNum;
                        strcpy(newDE[1].name, "..");
                        newDE[1].inodeNum = parentInodeNum;
                        DevWriteBlock(newBlockNum, (char *) newDE);
                        DevWriteBlock(bnum, (char *) directory_entry);

                        newInode->dirBlockPtr[0] = newBlockNum;
                        PutInode(newInodeNum, newInode);
                        free(newInode);
                        DevWriteBlock(indirectBlockNum, indirectBlock);
                        return 0;
                    }
                }
                return -1;

            }
        }
    }
    return -1;

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

    for (oh_array_index; oh_array_index < arr_index - 1; oh_array_index++) // 루트에 만들어지면 여기는 그냥 통과가 되버리네
    {
        *curInodeNum = goDownDir(arr[oh_array_index], pInode);// 이게 돌아주면 될것 같다
    }
    int parentInodeNum = *curInodeNum;
    *curInodeNum = goDownDir(arr[oh_array_index], pInode);// 이게 돌아주면 될것 같다
    if (*curInodeNum == -1) //no such directory
        return -1;

    char *pBlock = (char *) malloc(BLOCK_SIZE);

    ResetBlockBitmap(pInode->dirBlockPtr[0]);
    ResetInodeBitmap(*curInodeNum);
    DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
    pFileSysInfo->numAllocInodes--;
    DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
    DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
    pFileSysInfo->numAllocBlocks--;
    pFileSysInfo->numFreeBlocks++;
    DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
    Inode *parentInode = (Inode *) malloc(BLOCK_SIZE);
    GetInode(parentInodeNum, parentInode);
    //disLinkWithParent(parentInode, *curInodeNum, *curInodeNum);



    //direct directory_entry
    DirEntry directory_entry[NUM_OF_DIRENT_PER_BLOCK];
    for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {

        DevReadBlock(parentInode->dirBlockPtr[i], (char *) directory_entry);
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (directory_entry[j].inodeNum == *curInodeNum) {
                directory_entry[j].inodeNum = 0;
                memset(directory_entry[j].name, 0, sizeof(directory_entry[j].name));
                //strcpy(directory_entry[j].name,"");
                DevWriteBlock(parentInode->dirBlockPtr[i], (char *) directory_entry);

                //if number of directory_entry equal 0, also dislink directory_entry with parent;
                int numOfEmptyBlockInDE = 0;
                for (int i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++) {
                    if (directory_entry[i].inodeNum == 0)
                        numOfEmptyBlockInDE++;
                }
                if (numOfEmptyBlockInDE == 4) {
                    ResetBlockBitmap(parentInode->dirBlockPtr[i]);
                    DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
                    pFileSysInfo->numAllocBlocks--;
                    pFileSysInfo->numFreeBlocks++;
                    DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                    parentInode->dirBlockPtr[i] = 0;
                    parentInode->size -= BLOCK_SIZE;
                    PutInode(parentInodeNum, parentInode);
                }

                return 0;
            }
        }
    }
    int indirectBlock[BLOCK_SIZE / sizeof(int)];


    DevReadBlock(parentInode->indirBlockPtr, (char *) indirectBlock);
    for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++) {

        DevReadBlock(indirectBlock[i], (char *) directory_entry);
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (directory_entry[j].inodeNum == *curInodeNum) {
                directory_entry[j].inodeNum = 0;
                DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
                pFileSysInfo->numAllocInodes--;
                DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                ResetInodeBitmap(*curInodeNum);
                memset(directory_entry[j].name, 0, sizeof(directory_entry[j].name));
                //strcpy(directory_entry[j].name,"");
                DevWriteBlock(indirectBlock[i], directory_entry);

                int numOfEmptyBlockInDE = 0;
                for (int i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++) {
                    if (directory_entry[i].inodeNum == 0)
                        numOfEmptyBlockInDE++;
                }
                //directory_entry has no child, free block
                if (numOfEmptyBlockInDE == 4) {
                    ResetBlockBitmap(indirectBlock[i]);
                    DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
                    pFileSysInfo->numAllocBlocks--;
                    pFileSysInfo->numFreeBlocks++;
                    DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                    indirectBlock[i] = 0;
                    parentInode->size -= BLOCK_SIZE;
                    DevWriteBlock(parentInode->indirBlockPtr, (char *) indirectBlock);
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
                    DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
                    pFileSysInfo->numAllocInodes--;
                    DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                    parentInode->indirBlockPtr = 0;
                    PutInode(parentInodeNum, parentInode);
                }
                return 0;
            }
        }
    }


    free(pInode);
    free(curInodeNum);

    free(pBlock);
    free(parentInode);
    return 0;

}


int EnumerateDirStatus(const char *szDirName, DirEntryInfo *pDirEntry, int dirEntrys) {
    Inode *pInode = (Inode *) malloc(sizeof(Inode));
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
    GetInode(0, pInode);
    for (oh_array_index; oh_array_index < arr_index - 1; oh_array_index++) // 루트에 만들어지면 여기는 그냥 통과가 되버리네
    {
        *curInodeNum = goDownDir(arr[oh_array_index], pInode);// 이게 돌아주면 될것 같다
    }
    int parentInodeNum = *curInodeNum;
    *curInodeNum = goDownDir(arr[oh_array_index], pInode);// 이게 돌아주면 될것 같다
    if (parentInodeNum == -1 || *curInodeNum == -1) {
        free(pInode);
        free(curInodeNum);
        return -1;
    }

    Inode *parentInode = (Inode *) malloc(sizeof(Inode));
    GetInode(parentInodeNum, parentInode);
    int  count = 0;


    //direct
    DirEntry directory_entry[NUM_OF_DIRENT_PER_BLOCK];
    for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {

        if (pInode->dirBlockPtr[i] == 0)
            continue;
        DevReadBlock(pInode->dirBlockPtr[i], (char *) directory_entry);
        Inode *tempInode = (Inode *) malloc(sizeof(Inode));

        for (int i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++) {
            if (count >= dirEntrys)
                break;
            if (strcmp(directory_entry[i].name, "") == 0)
                continue;
            pDirEntry[count].inodeNum = directory_entry[i].inodeNum;
            strcpy(pDirEntry[count].name, directory_entry[i].name);
            GetInode(directory_entry[i].inodeNum, tempInode);
            pDirEntry[count].type = (FileType) tempInode->type;
            count++;

        }

        free(tempInode);
        if (count >= dirEntrys) {

            free(pInode);
            free(curInodeNum);

            free(parentInode);
            return count;
        }
    }

    //indirect

    if (pInode->indirBlockPtr != 0) {
        int indirectBlock[BLOCK_SIZE / sizeof(int)];
        DevReadBlock(pInode->indirBlockPtr, (char *) indirectBlock);
        for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++) {

            if (indirectBlock[i] == 0)
                continue;
            DevReadBlock(indirectBlock[i], (char *) directory_entry);
            Inode *tempInode = (Inode *) malloc(sizeof(Inode));

            for (int i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++) {
                if (count >= dirEntrys)
                    break;
                if (strcmp(directory_entry[i].name, "") == 0)
                    continue;
                pDirEntry[count].inodeNum = directory_entry[i].inodeNum;
                strcpy(pDirEntry[count].name, directory_entry[i].name);
                GetInode(directory_entry[i].inodeNum, tempInode);
                pDirEntry[count].type = (FileType) tempInode->type;
                count++;

            }

            free(tempInode);
            if (count >= dirEntrys) {
                free(pInode);
                free(curInodeNum);

                free(parentInode);
                return count;
            }
        }
    }
    free(pInode);
    free(curInodeNum);

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