//-fs.c: 과제1에서 구현한 코드를 이 파일에 넣고, 빈 함수를 구현하면 됩니다.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "disk.h"
#include "fs.h"
#include <math.h>

FileDescTable *pFileDescTable = NULL;
DirEntry myDirPtr;
int myDirPtr_index;
int myblock_index;

int findNameNotNULL(const char *currentFileName, int inode_index, DirEntry *returnDirPtr, int *returnDirPtr_index,
                    int *return_block_index);

int findNameNotNULL(const char *currentFileName, int inode_index, DirEntry *returnDirPtr, int *returnDirPtr_index,
                    int *return_block_index) {
    Inode *current_root = (Inode *) malloc(sizeof(Inode));
    GetInode(inode_index, current_root);

    char *block_buf = (char *) malloc(BLOCK_SIZE);
    for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {//다이렉트 노트 확인
        DevReadBlock(current_root->dirBlockPtr[i], block_buf);
        if (current_root->dirBlockPtr[i] == 0) {
            break;
        }
        DirEntry *dir_block = (DirEntry *) block_buf;
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (strcmp(dir_block[j].name, currentFileName) == 0) {
                for (int dir_index = 0; dir_index < NUM_OF_DIRENT_PER_BLOCK; dir_index++) {
                    returnDirPtr[dir_index].inodeNum = dir_block[dir_index].inodeNum;
                    strcpy(returnDirPtr[dir_index].name, dir_block[dir_index].name);
                }
                *returnDirPtr_index = j;
                *return_block_index = current_root->dirBlockPtr[i];
                myDirPtr_index = j;
                myblock_index = current_root->dirBlockPtr[i];
                return 1;
            }

        }
    }
    if (current_root->indirBlockPtr != 0) {
        char *block_indirect_buf = (char *) malloc(BLOCK_SIZE);
        //get indirect block, cast to indirect block
        DevReadBlock(current_root->indirBlockPtr, block_buf);
        int *indirect = (int *) block_buf;

        for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
            if (indirect[i] == 0) {
                break;
            } else {
                DevReadBlock(indirect[i], block_indirect_buf);
            }

            DirEntry *dir_block = (DirEntry *) block_indirect_buf;


            for (int blockindex = 0; blockindex < NUM_OF_DIRENT_PER_BLOCK; blockindex++) {
                if (strcmp(dir_block[blockindex].name, currentFileName) == 0) {
                    for (int dir_index = 0; dir_index < NUM_OF_DIRENT_PER_BLOCK; dir_index++) {
                        returnDirPtr[dir_index].inodeNum = dir_block[dir_index].inodeNum;
                        strcpy(returnDirPtr[dir_index].name, dir_block[dir_index].name);
                    }
                    *returnDirPtr_index = blockindex;
                    *return_block_index = indirect[i];
                    myDirPtr_index = blockindex;
                    myblock_index = indirect[i];
                    return 1;
                }

            }
        }
    }

    return 0;
}

int
findNameNull(int inode_index, DirEntry *returnDirPtr, int *returnDirPtr_index, int *return_block_index) {

    Inode *current_root = (Inode *) malloc(sizeof(Inode));
    GetInode(inode_index, current_root);

    char *block_buf = (char *) malloc(BLOCK_SIZE);

    for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {//다이렉트 노트 확인

        DevReadBlock(current_root->dirBlockPtr[i], block_buf);
        if (current_root->dirBlockPtr[i] == 0) {
            break;
        }
        DirEntry *dir_block = (DirEntry *) block_buf;
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (strcmp(dir_block[j].name, "") == 0) {
                for (int dir_index = 0; dir_index < NUM_OF_DIRENT_PER_BLOCK; dir_index++) {
                    returnDirPtr[dir_index].inodeNum = dir_block[dir_index].inodeNum;
                    strcpy(returnDirPtr[dir_index].name, dir_block[dir_index].name);
                }
                *returnDirPtr_index = j;
                *return_block_index = current_root->dirBlockPtr[i];

                return 1;
            }
        }
    }
    if (current_root->indirBlockPtr != 0) {
        DevReadBlock(current_root->indirBlockPtr, block_buf);
        int *indirect = (int *) block_buf;
        char *block_indirect_buf = (char *) malloc(BLOCK_SIZE);

        for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
            DevReadBlock(indirect[i], block_indirect_buf);
            if (indirect[i] == 0)
                break;
            DirEntry *dir_block = (DirEntry *) block_indirect_buf;
            for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
                if (strcmp(dir_block[j].name, "") == 0) {
                    for (int dir_index = 0; dir_index < NUM_OF_DIRENT_PER_BLOCK; dir_index++) {
                        returnDirPtr[dir_index].inodeNum = dir_block[dir_index].inodeNum;
                        strcpy(returnDirPtr[dir_index].name, dir_block[dir_index].name);
                    }
                    *returnDirPtr_index = j;
                    *return_block_index = indirect[i];

                    return 1;
                }
            }
        }
    }


    return 0;
}

int OpenFile(const char *szFileName, OpenFlag flag) {
    int inode_index = 0;

    //for save string

    char arr[64][MAX_NAME_LEN + 1]; //폴더 갯수 최대 그냥 64로 했음
    int arr_index = 0;
    //ㅎㅈ쓰거
    char *input_path = (char *) malloc(sizeof(szFileName));
    strcpy(input_path, szFileName);
    char *temp_Ptr = strtok(input_path, "/");

    while (temp_Ptr != NULL) {
        strcpy(arr[arr_index++], temp_Ptr);
        temp_Ptr = strtok(NULL, "/");
    }
    int depth = 0;
    DirEntry *returnDirPtr = (DirEntry *) malloc(BLOCK_SIZE);
    int returnDirPtr_index = 0;
    int return_block_index = 19;
    //find
    while (findNameNotNULL(arr[depth], inode_index, returnDirPtr, &returnDirPtr_index, &return_block_index)) {
        if (arr_index == depth + 1) {
            FileDesc *fdPtr = (FileDesc *) pFileDescTable;
            for (int i = 0; i < MAX_FD_ENTRY_LEN; i++) {
                if (fdPtr[i].bUsed == 0) {
                    //set inode_index
                    fdPtr[i].inodeNum = returnDirPtr[returnDirPtr_index].inodeNum;
                    fdPtr[i].fileOffset = 0;
                    fdPtr[i].bUsed = 1;
                    return i;
                }
            }
        }
        inode_index = returnDirPtr[returnDirPtr_index].inodeNum;
        depth++;
    }
    if (flag == OPEN_FLAG_CREATE && depth < arr_index) {
        int temp = findNameNull(inode_index, returnDirPtr, &returnDirPtr_index, &return_block_index);//이거 용도 뭐지
//        findNameNULL(&inode_index,retDirPtr, &retDirPtrIndex, &retBlkIndex, &is_full);
        DirEntry *pDirEntry = returnDirPtr;
        int pDirPtrIndex = returnDirPtr_index;
        int parent_inode_index = inode_index;
        int parent_block_index = return_block_index;
        if (temp == 0) //찼을때
        {
            //get parent inoPtr
            Inode *pInoPtr = (Inode *) malloc(sizeof(Inode));
            GetInode(parent_inode_index, pInoPtr);

            if (pInoPtr->dirBlockPtr[1] == 0) {
                //init pDirEntry, pDirPtrIndex
                memset(pDirEntry, 0, BLOCK_SIZE);
                pDirPtrIndex = 0;
                //link dirPtr with parent inoPtr, write to disk, modify parent_block_index
                int pFreeBlkIndex = GetFreeBlockNum();
                pInoPtr->dirBlockPtr[1] = pFreeBlkIndex;
                PutInode(parent_inode_index, pInoPtr);
                parent_block_index = pFreeBlkIndex;
                //set bitmap
                SetBlockBitmap(pFreeBlkIndex);
                //update file sys info
                DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                pFileSysInfo->numAllocBlocks++;
                pFileSysInfo->numFreeBlocks--;
                DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                //printf("myMakeDir()... if full...parent_inode_index = %d, pFreeBlkIndex = %d...\n", parent_inode_index, pFreeBlkIndex);
            } else {
                if (pInoPtr->indirBlockPtr == 0) {
                    //init pDirEntry, pDirPtrIndex
                    memset(pDirEntry, 0, BLOCK_SIZE);
                    pDirPtrIndex = 0;
                    //link indirPtr with parent inoPtr
                    int pFreeBlkIndex = GetFreeBlockNum();
                    pInoPtr->indirBlockPtr = pFreeBlkIndex;
                    PutInode(parent_inode_index, pInoPtr);
                    //set bitmap
                    SetBlockBitmap(pFreeBlkIndex);
                    //update file sys info for indirPtr
                    DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                    pFileSysInfo->numAllocBlocks++;
                    pFileSysInfo->numFreeBlocks--;
                    DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                    //printf("myMakeDir()... if full...parent_inode_index = %d, pInoPtr->indirBlockPtr = %d, pFreeBlkIndex = %d...\n", parent_inode_index, pInoPtr->indirBlockPtr ,pFreeBlkIndex);

                    //get & set indirPtr, write to disk
                    char *pIndirBlkPtr = (char *) malloc(BLOCK_SIZE);
                    memset(pIndirBlkPtr, 0, BLOCK_SIZE);
                    int *pIndirPtr = (int *) pIndirBlkPtr;
                    int pFreeBlkIndex2 = GetFreeBlockNum();
                    pIndirPtr[0] = pFreeBlkIndex2;
                    DevWriteBlock(pFreeBlkIndex, (char *) pIndirPtr);
                    //set bitmap
                    SetBlockBitmap(pFreeBlkIndex2);
                    //update file sys info for indirPtr
                    DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                    pFileSysInfo->numAllocBlocks++;
                    pFileSysInfo->numFreeBlocks--;
                    DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                    //printf("myMakeDir()... if full... parent_inode_index = %d, pFreeBlkIndex2 = %d, pIndirPtr[0] = %d...\n", parent_inode_index, pFreeBlkIndex2, pIndirPtr[0]);

                    //modify parent_block_index
                    parent_block_index = pFreeBlkIndex2;
                    //free
                    free(pIndirBlkPtr);
                } else {
                    //get indirPtr, link dirPtr with parent inoPtr, write to disk, modify parent_block_index
                    char *tmpBlk = (char *) malloc(BLOCK_SIZE);
                    DevReadBlock(pInoPtr->indirBlockPtr, tmpBlk);
                    int *indirPtr = (int *) tmpBlk;

                    for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
                        if (indirPtr[i] == 0) {
                            //init pDirEntry, pDirPtrIndex
                            memset(pDirEntry, 0, BLOCK_SIZE);
                            pDirPtrIndex = 0;
                            int pFreeBlkIndex = GetFreeBlockNum();
                            indirPtr[i] = pFreeBlkIndex;
                            DevWriteBlock(pInoPtr->indirBlockPtr, (char *) indirPtr);
                            parent_block_index = pFreeBlkIndex;
                            //set bitmap
                            SetBlockBitmap(pFreeBlkIndex);
                            //update file sys info
                            DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                            pFileSysInfo->numAllocBlocks++;
                            pFileSysInfo->numFreeBlocks--;
                            DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                            //printf("myMakeDir()... if full & indirect != 0... indirPtr[%d] = %d, parent_block_index = %d\n", i, indirPtr[i], parent_block_index);
                        }
                    }
                    free(tmpBlk);
                }
            }
            //free
            free(pInoPtr);
        }
        int cFreeInoIndex = GetFreeInodeNum();
        //int cFreeBlkIndex = GetFreeBlockNum();

        //set parent's DirEntry, write to disk
        strcpy(pDirEntry[pDirPtrIndex].name, arr[arr_index - 1]);
        pDirEntry[pDirPtrIndex].inodeNum = cFreeInoIndex;
        DevWriteBlock(parent_block_index, (char *) pDirEntry);
        printf("myMakeFile()... pDirEntry[%d].name = %s, ino = %d...\n", pDirPtrIndex, pDirEntry[pDirPtrIndex].name,
               pDirEntry[pDirPtrIndex].inodeNum);
        //get created newly created dir's inode, set data, write to disk
        Inode *inoPtr = (Inode *) malloc(sizeof(Inode));
        GetInode(cFreeInoIndex, inoPtr);
        inoPtr->type = FILE_TYPE_FILE;
        inoPtr->size = 0;
        inoPtr->dirBlockPtr[0] = 0;
        inoPtr->dirBlockPtr[1] = 0;
        inoPtr->indirBlockPtr = 0;
        PutInode(cFreeInoIndex, inoPtr);

        //set bitmap
        SetInodeBitmap(cFreeInoIndex);

        //get sys info, write to disk
        DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
        pFileSysInfo->numAllocInodes++;
        DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);

        //set fd table
        FileDesc *fdPtr = (FileDesc *) pFileDescTable;
        int fFdIndex = 0;
        for (int i = 0; i < MAX_FD_ENTRY_LEN; i++) {
            //if find free fd index
            if (fdPtr[i].bUsed == 0) {
                fFdIndex = i;
                fdPtr[i].bUsed = 1;
                fdPtr[i].fileOffset = 0;
                fdPtr[i].inodeNum = cFreeInoIndex;
                //printf("fdPtr[%d].bUsed = %d, offset = %d, ino = %d...\n\n", i, fdPtr[i].bUsed, fdPtr[i].fileOffset, fdPtr[i].inodeNum);
                break;
            }
        }

        //free
        free(inoPtr);
        return fFdIndex;
    } else {
        return -1;
    }

}

int WriteFile(int fileDesc, char *pBuffer, int length) {

}

int ReadFile(int fileDesc, char *pBuffer, int length) {

}

int CloseFile(int fileDesc) {
    FileDesc *fdPtr = (FileDesc *) pFileDescTable;

    if (fdPtr[fileDesc].bUsed != 0) {
        fdPtr[fileDesc].inodeNum = 0;
        fdPtr[fileDesc].fileOffset = 0;
        fdPtr[fileDesc].bUsed = 0;
        return 0;
    }
    return -1;
}

int RemoveFile(const char *szFileName) {

}


int MakeDir(const char *szDirName) {
    int inode_index = 0;

    //for save string

    char arr[64][MAX_NAME_LEN + 1]; //폴더 갯수 최대 그냥 64로 했음
    int arr_index = 0;
    //ㅎㅈ쓰거
    char *input_path = (char *) malloc(sizeof(szDirName));
    strcpy(input_path, szDirName);
    char *temp_Ptr = strtok(input_path, "/");

    while (temp_Ptr != NULL) {
        strcpy(arr[arr_index++], temp_Ptr);
        temp_Ptr = strtok(NULL, "/");
    }
    int depth = 0;
    DirEntry *returnDirPtr = (DirEntry *) malloc(BLOCK_SIZE);
    int returnDirPtr_index = 0;
    int return_block_index = 19;
    //find
    while (findNameNotNULL(arr[depth], inode_index, returnDirPtr, &returnDirPtr_index, &return_block_index)) {
        inode_index = returnDirPtr[returnDirPtr_index].inodeNum;
        depth++;
    }
    int temp = findNameNull(inode_index, returnDirPtr, &returnDirPtr_index, &return_block_index);//이거 용도 뭐지
    DirEntry *pDirEntry = returnDirPtr;
    int pDirPtrIndex = returnDirPtr_index;
    int parent_inode_index = inode_index;
    int parent_block_index = return_block_index;

    if (temp == 0) {
        memset(pDirEntry, 0, BLOCK_SIZE);
        pDirPtrIndex = 0;
        Inode *parent_inode_buf = (Inode *) malloc(sizeof(Inode));
        GetInode(parent_inode_index, parent_inode_buf);
        if (parent_inode_buf->dirBlockPtr[1] == 0) {
            parent_inode_buf->dirBlockPtr[1] = GetFreeBlockNum();
            PutInode(parent_inode_index, parent_inode_buf);
            parent_block_index = GetFreeBlockNum();
            SetBlockBitmap(GetFreeBlockNum());
            DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
            pFileSysInfo->numAllocBlocks++;
            pFileSysInfo->numFreeBlocks--;
            DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
        } else {
            if (parent_inode_buf->indirBlockPtr == 0) {

                int parent_free_block_index = GetFreeBlockNum();
                parent_inode_buf->indirBlockPtr = parent_free_block_index;
                PutInode(parent_inode_index, parent_inode_buf);
                SetBlockBitmap(parent_free_block_index);
                DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                pFileSysInfo->numAllocBlocks++;
                pFileSysInfo->numFreeBlocks--;
                DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                char *pIndirBlkPtr = (char *) malloc(BLOCK_SIZE);
                int *pIndirPtr = (int *) pIndirBlkPtr;
                pIndirPtr[0] = GetFreeBlockNum();
                DevWriteBlock(parent_free_block_index, (char *) pIndirPtr);
                DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                pFileSysInfo->numAllocBlocks++;
                pFileSysInfo->numFreeBlocks--;
                DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                parent_block_index = GetFreeBlockNum();
                SetBlockBitmap(GetFreeBlockNum());

            } else {
                char *tmpBlk = (char *) malloc(BLOCK_SIZE);
                DevReadBlock(parent_inode_buf->indirBlockPtr, tmpBlk);
                int *indirPtr = (int *) tmpBlk;

                for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
                    if (indirPtr[i] == 0) {
                        indirPtr[i] = GetFreeBlockNum();
                        DevWriteBlock(parent_inode_buf->indirBlockPtr, (char *) indirPtr);
                        parent_block_index = GetFreeBlockNum();
                        SetBlockBitmap(GetFreeBlockNum());
                        DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                        pFileSysInfo->numAllocBlocks++;
                        pFileSysInfo->numFreeBlocks--;
                        DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                    }
                }
            }
        }

    }
    strncpy(pDirEntry[pDirPtrIndex].name, arr[depth],
            sizeof(pDirEntry[pDirPtrIndex].name) - 1); //strcpy는 안좋다니까 strncpy로 함
    pDirEntry[pDirPtrIndex].inodeNum = GetFreeInodeNum();
    DevWriteBlock(parent_block_index, (char *) pDirEntry);
    char *current_block_buf = (char *) malloc(BLOCK_SIZE);
    memset(current_block_buf, 0, BLOCK_SIZE);
    DirEntry *cDirEntry = (DirEntry *) current_block_buf;
    strcpy(cDirEntry[0].name, ".");
    cDirEntry[0].inodeNum = GetFreeInodeNum();
    strcpy(cDirEntry[1].name, "..");
    cDirEntry[1].inodeNum = parent_inode_index;
    DevWriteBlock(GetFreeBlockNum(), (char *) cDirEntry);
    Inode *current_inode_buf = (Inode *) malloc(sizeof(Inode));
    memset(current_inode_buf, 0, sizeof(Inode));
    GetInode(GetFreeInodeNum(), current_inode_buf);
    current_inode_buf->type = FILE_TYPE_DIR;
    current_inode_buf->size = 0;
    current_inode_buf->dirBlockPtr[0] = GetFreeBlockNum();
    current_inode_buf->dirBlockPtr[1] = 0;
    current_inode_buf->indirBlockPtr = 0;
    PutInode(GetFreeInodeNum(), current_inode_buf);
    SetInodeBitmap(GetFreeInodeNum());
    SetBlockBitmap(GetFreeBlockNum());
    DevReadBlock(FILESYS_INFO_BLOCK, current_block_buf);
    FileSysInfo *fileSysInfo = (FileSysInfo *) current_block_buf;
    fileSysInfo->numAllocBlocks++;
    fileSysInfo->numFreeBlocks--;
    fileSysInfo->numAllocInodes++;
    DevWriteBlock(FILESYS_INFO_BLOCK, current_block_buf);
    return 0;
}

int RemoveDir(const char *szDirName) {
    int inode_index = 0;
    char arr[64][MAX_NAME_LEN + 1]; //폴더 갯수 최대 그냥 64로 했음
    int arr_index = 0;
    char *input_path = (char *) malloc(sizeof(szDirName));
    strcpy(input_path, szDirName);
    char *temp_Ptr = strtok(input_path, "/");
    while (temp_Ptr != NULL) {
        strcpy(arr[arr_index++], temp_Ptr);
        temp_Ptr = strtok(NULL, "/");
    }
    int depth = 0;
    DirEntry *returnDirPtr = (DirEntry *) malloc(BLOCK_SIZE);
    int returnDirPtr_index = 0;
    int return_block_index = 19;
    for (int i = 0; i < arr_index - 1; i++) // 루트에 만들어지면 여기는 그냥 통과가 되버리네
    {

    }
    while (findNameNotNULL(arr[depth], inode_index, returnDirPtr, &returnDirPtr_index, &return_block_index)) {
        if (arr_index == depth + 1) {
            break;

        }
        inode_index = returnDirPtr[returnDirPtr_index].inodeNum;
        depth++;
    }
    DirEntry *pDirEntry = returnDirPtr;
    int pDirPtrIndex = returnDirPtr_index;
    int parent_inode_index = inode_index;
    int parent_block_index = return_block_index;//이거 바꿔야겠다.


    Inode *current_inode_buf = (Inode *) malloc(sizeof(Inode));
    int current_inode_index = pDirEntry[pDirPtrIndex].inodeNum;
    GetInode(current_inode_index, current_inode_buf);
    char *tmpBlk = (char *) malloc(BLOCK_SIZE);
    //check if child dir is empty at direct
    for (int index = 0; index < NUM_OF_DIRECT_BLOCK_PTR; index++) {
        //get dirBlk, cast to DirEntry*
        if (current_inode_buf->dirBlockPtr[index] != 0) {
            DevReadBlock(current_inode_buf->dirBlockPtr[index], tmpBlk);
            DirEntry *dir_block = (DirEntry *) tmpBlk;

            for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {

                if ((strcmp(dir_block[j].name, ".") != 0 && strcmp(dir_block[j].name, "..") != 0) &&
                    strcmp(dir_block[j].name, "") != 0) {
                    return 0;
                }
            }
        }


    }
    char *tmpBlk2 = (char *) malloc(BLOCK_SIZE);

    //check if child dir is empty at indirect
    if (current_inode_buf->indirBlockPtr != 0) {
        //get indirPtr, cast to int*
        DevReadBlock(current_inode_buf->indirBlockPtr, tmpBlk);
        int *indirPtr = (int *) tmpBlk;

        for (int k = 0; k < BLOCK_SIZE / sizeof(int); k++) {
            //if not empty dir block
            if (indirPtr[k] != 0) {
                //get block, cast to DirEntry*
                DevReadBlock(indirPtr[k], tmpBlk2);
                DirEntry *dir_block = (DirEntry *) tmpBlk2;

                for (int l = 0; l < NUM_OF_DIRENT_PER_BLOCK; l++) {
                    if (strcmp(dir_block[l].name, "") != 0)
                        break;
                    if (l == NUM_OF_DIRENT_PER_BLOCK - 1)
                        return 0;
                }
            }

        }
    }


    {
        memset(tmpBlk, 0, BLOCK_SIZE);
        DevWriteBlock(current_inode_buf->dirBlockPtr[0], tmpBlk);
        memset(current_inode_buf, 0, sizeof(Inode));
        PutInode(pDirEntry[pDirPtrIndex].inodeNum, current_inode_buf);
        ResetBlockBitmap(current_inode_buf->dirBlockPtr[0]);
        ResetInodeBitmap(pDirEntry[pDirPtrIndex].inodeNum);
        DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
        pFileSysInfo->numAllocBlocks--;
        pFileSysInfo->numFreeBlocks++;
        pFileSysInfo->numAllocInodes--;

        strcpy(pDirEntry[pDirPtrIndex].name, "");
        pDirEntry[pDirPtrIndex].inodeNum = 0;
        DevWriteBlock(parent_block_index, (char *) pDirEntry);


        Inode *parent_inode_buf = (Inode *) malloc(sizeof(Inode));
        GetInode(parent_inode_index, parent_inode_buf);
        if (parent_inode_buf->dirBlockPtr[1] == parent_block_index) {
            parent_inode_buf->dirBlockPtr[1] = 0;
            memset(tmpBlk, 0, BLOCK_SIZE);
            DevWriteBlock(parent_block_index, tmpBlk);
            ResetBlockBitmap(parent_block_index);
            DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
            pFileSysInfo->numAllocBlocks--;
            pFileSysInfo->numFreeBlocks++;
            DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
        } else if (parent_inode_buf->indirBlockPtr != 0) {
            DevReadBlock(parent_inode_buf->indirBlockPtr, tmpBlk);
            int *indirPtr = (int *) tmpBlk;
            for (int indirec_index = 0; indirec_index < BLOCK_SIZE / sizeof(int); indirec_index++) {
                if (indirPtr[indirec_index] == parent_block_index) {
                    indirPtr[indirec_index] = 0;
                    memset(tmpBlk2, 0, BLOCK_SIZE);
                    DevWriteBlock(parent_block_index, tmpBlk2);
                    ResetBlockBitmap(parent_block_index);
                    DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                    pFileSysInfo->numAllocBlocks--;
                    pFileSysInfo->numFreeBlocks++;
                    DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                    ResetBlockBitmap(parent_inode_buf->indirBlockPtr);
                    DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                    pFileSysInfo->numAllocBlocks--;
                    pFileSysInfo->numFreeBlocks++;
                    DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                    memset(tmpBlk2, 0, BLOCK_SIZE);
                    DevWriteBlock(parent_inode_buf->indirBlockPtr, tmpBlk2);
                    parent_inode_buf->indirBlockPtr = 0;
                }

            }

        }


        //update fsInfo
        DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
    }

    return -1;
}

int EnumerateDirStatus(const char *szDirName, DirEntryInfo *pDirEntry, int dirEntrys) {
    int inode_index = 0;
    int count = 0;

    char arr[64][MAX_NAME_LEN + 1]; //폴더 갯수 최대 그냥 64로 했음
    int arr_index = 0;
    //ㅎㅈ쓰거
    char *input_path = (char *) malloc(sizeof(szDirName));
    strcpy(input_path, szDirName);
    char *temp_Ptr = strtok(input_path, "/");

    while (temp_Ptr != NULL) {
        strcpy(arr[arr_index++], temp_Ptr);
        temp_Ptr = strtok(NULL, "/");
    }
    int depth = 0;
    DirEntry *returnDirPtr = (DirEntry *) malloc(BLOCK_SIZE);
    int returnDirPtr_index = 0;
    int return_block_index = 19;
    //find
    while (findNameNotNULL(arr[depth], inode_index, returnDirPtr, &returnDirPtr_index, &return_block_index)) {

        if (arr_index == depth + 1) {
            break;
        }
        depth++;
        inode_index = returnDirPtr[returnDirPtr_index].inodeNum;
    }
    //if found dir name to make
    //get inode
    Inode *inode_buf = (Inode *) malloc(sizeof(Inode));
    Inode *current_inode_buf = (Inode *) malloc(sizeof(Inode));
    GetInode(returnDirPtr[returnDirPtr_index].inodeNum, inode_buf);
    char *tmpBlk = (char *) malloc(BLOCK_SIZE);

    //get DirEntryInfo at direct
    for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {
        if (inode_buf->dirBlockPtr[i] != 0) {
            //get dir block, cast to dirEntry*
            DevReadBlock(inode_buf->dirBlockPtr[i], tmpBlk);
            DirEntry *dir_block = (DirEntry *) tmpBlk;

            //for all dir entry
            for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
                if (strcmp(dir_block[j].name, "") != 0) {
                    //get child inode to get type
                    GetInode(dir_block[j].inodeNum, current_inode_buf);
                    pDirEntry[count].type = (FileType) current_inode_buf->type;
                    strcpy(pDirEntry[count].name, dir_block[j].name);
                    pDirEntry[count].inodeNum = dir_block[j].inodeNum;
                    count++;
                }
            }
        }
    }
    //get DirEntryInfo at indirect
    if (inode_buf->indirBlockPtr != 0) {
        //get indir block, cast to DirEntry*
        DevReadBlock(inode_buf->indirBlockPtr, tmpBlk);
        int *indirPtr = (int *) tmpBlk;
        for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
            if (indirPtr[i] != 0) {
                //get data block, cast to DirEntry*
                char *tmpBlk2 = (char *) malloc(BLOCK_SIZE);
                DevReadBlock(indirPtr[i], tmpBlk2);
                DirEntry *dir_block = (DirEntry *) tmpBlk2;
                //for all dir entry
                for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
                    if (strcmp(dir_block[j].name, "") != 0) {
                        //get child inode to get type
                        GetInode(dir_block[j].inodeNum, current_inode_buf);
                        pDirEntry[count].type = (FileType) current_inode_buf->type;
                        strcpy(pDirEntry[count].name, dir_block[j].name);
                        pDirEntry[count].inodeNum = dir_block[j].inodeNum;
                        count++;
                    }
                }
            }
        }
    }

    return count;


    return -1;
}

void FileSysInit(void) //Success
{
    DevCreateDisk();
    // //DevOpenDisk();

    char *buf = malloc(BLOCK_SIZE);
    // for (int i = 0; i < BLOCK_SIZE; i++)
    // {
    //     buf[i] = 0;
    // }
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
    //
    //    char *buf = malloc(BLOCK_SIZE);
    //    //DevOpenDisk();
    //    DevReadBlock((inodeno / 8) + INODELIST_BLOCK_FIRST, buf);
    //    memcpy(pInode, buf + (inodeno % 8) * sizeof(Inode), sizeof(Inode));
    //    free(buf);
}

int GetFreeInodeNum(void) {
    char *buf = malloc(BLOCK_SIZE);
    ////DevOpenDisk();
    DevReadBlock(INODE_BITMAP_BLOCK_NUM, buf);

    for (int i = 0; i < BLOCK_SIZE; i++) //이러면 24부터네
    {
        if (buf[i] == -1) // if 11111111 => continue
            continue;
        for (int j = 0; j < 8; j++) {
            if ((buf[i] << j & 128) == 0) // >> 연산자가 & 보다 우선순위가 높구나
            {
                free(buf);
                return (i * 8) + j;
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
    for (int j = 3; j < 8; j++) {
        if ((buf[i] << j & 128) == 0) // >> 연산자가 & 보다 우선순위가 높구나
        {
            free(buf);
            return (i * 8) + j;
        }
    }
    for (int i = 3; i < BLOCK_SIZE; i++) {
        if (buf[i] == -1) // if 11111111 => continue
            continue;

        for (int j = 0; j < 8; j++) {
            if ((buf[i] << j & 128) == 0) // >> 연산자가 & 보다 우선순위가 높구나
            {
                free(buf);
                return (i * 8) + j;
            }
        }
    }
    free(buf);
    return -1; //실패 했을 경우
}
