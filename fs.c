//-fs.c: 과제1에서 구현한 코드를 이 파일에 넣고, 빈 함수를 구현하면 됩니다.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include "disk.h"
#include "fs.h"

FileDescTable *pFileDescTable = NULL;
typedef struct {
    char **array;
    size_t used;
    size_t capa;
} Matrix;

void initMatrix(Matrix *m, size_t initCapa);

void insertMatrix(Matrix *m, char *element);

void printMatrix(Matrix *m);

void freeMatrix(Matrix *m);

void initMatrix(Matrix *m, size_t initCapa) {
    m->array = (char **) malloc(initCapa * sizeof(char *));
    memset(m->array, 0, initCapa * sizeof(char *));
    m->used = 0;
    m->capa = initCapa;
}

void insertMatrix(Matrix *m, char *element) {
    if (m->used == m->capa) {
        m->capa *= 2;
        m->array = (char **) realloc(m->array, m->capa * sizeof(char *));
    }
    m->array[m->used] = (char *) malloc(sizeof(element) + 1);
    memset(m->array[m->used], 0, sizeof(element) + 1);
    strcpy(m->array[m->used++], element);
}

void printMatrix(Matrix *m) {
    for (int i = 0; i < m->capa; i++) {
        printf("printMatrix...m->array[%d] = %s\n", i, m->array[i]);
    }
}

void freeMatrix(Matrix *m) {
    for (int i = 0; i < m->capa; i++) {
        if (m->array[i] != NULL) free(m->array[i]);
    }
    free(m->array);
    m->array = NULL;
    m->used = m->capa = 0;
}

void copyDirEnt(DirEntry *target, DirEntry *source) {
    for (int i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++) {
        strcpy(target[i].name, source[i].name);
        target[i].inodeNum = source[i].inodeNum;
    }
}

bool findName(int *inoIndex, Matrix *m, int depth, DirEntry *retDirPtr, int *retDirPtrIndex, int *retBlkIndex,
              bool *is_full) {
    //stop condition
    if (m != NULL && m->array[depth] == NULL)
        return false;
    if (m != NULL && (strcmp(m->array[depth], "") != 0))
        printf("\nsearching name : %s\n", m->array[depth]);
    *is_full = false;
    //get current inode
    Inode *root = (Inode *) malloc(sizeof(Inode));
    GetInode(*inoIndex, root);
    //printf("findName start...inoIndex = %d, retDirPtrIndex = %d, retBlkIndex = %d\n", *inoIndex, *retDirPtrIndex, *retBlkIndex);
    //alloc memory
    char *blkPtr = (char *) malloc(BLOCK_SIZE);
    char *blkPtr2 = (char *) malloc(BLOCK_SIZE);
    //check file name is already exist
    //at direct ptr
    for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {
        //read blcok, cast to DirEntry
        //check dirBlkPtr is 0 => if 0, no more meaningful block is alloced
        if (root->dirBlockPtr[i] != 0)
            DevReadBlock(root->dirBlockPtr[i], blkPtr);
        else {
            //printf("root->dirBlockPtr[%d] = %d\n", i, root->dirBlockPtr[i]);
            break;
        }
        DirEntry *dirPtr = (DirEntry *) blkPtr;

        //compare paresed name with DirEntry's name
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (m == NULL) {
                printf("eee...[%d][%d].name = '%s'\n", i, j, dirPtr[j].name);
                //if file name is ""
                if (strcmp(dirPtr[j].name, "") == 0) {
                    //set retDirPtr, retDirPtrIndex, retBlkIndex
                    copyDirEnt(retDirPtr, dirPtr);
                    *retDirPtrIndex = j;
                    *retBlkIndex = root->dirBlockPtr[i];
                    *is_full = false;
                    //printf("ccc...inoIndex = %d, retDirPtrIndex = %d, retBlkIndex = %d\n", *inoIndex, *retDirPtrIndex, *retBlkIndex);
                    //free
                    free(root);
                    free(blkPtr);

                    return true;
                } else if (j == NUM_OF_DIRENT_PER_BLOCK - 1)
                    *is_full = true;
            } else {
                //printf("zzz...[%d][%d].name = %s\n",i , j, dirPtr[j].name);
                //if file is already exist
                if (strcmp(dirPtr[j].name, m->array[depth]) == 0) {
                    //set retDirPtr, retDirPtrIndex
                    *inoIndex = dirPtr[j].inodeNum;
                    copyDirEnt(retDirPtr, dirPtr);
                    *retDirPtrIndex = j;
                    *retBlkIndex = root->dirBlockPtr[i];
                    //printf("yyy... inoIndex = %d, root->dirBloPtr[0] = %d, [1] = %d, indirect = %d\n",*inoIndex, root->dirBlockPtr[0],root->dirBlockPtr[1],root->indirBlockPtr);

                    // Inode* tmp = (Inode*)malloc(sizeof(Inode));
                    // GetInode(*inoIndex, tmp);
                    // printf("tmp->dir[0] = %d, [1] = %d, indir = %d\n", tmp->dirBlockPtr[0], tmp->dirBlockPtr[1], tmp->indirBlockPtr);

                    //free
                    free(root);
                    free(blkPtr);

                    return true;
                }
            }
        }
    }
    //check indir pointer in not 0
    if (root->indirBlockPtr != 0) {
        //get indirect block, cast to indirect block
        DevReadBlock(root->indirBlockPtr, blkPtr);
        int *indirect = (int *) blkPtr;

        //check indirect block
        for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
            //printf("xxx...indirect[%d] = %d\n", i, indirect[i]);
            //read blcok, cast to DirEntry
            //check indirect[i] is 0 => if 0, no more block is alloced
            if (indirect[i] != 0)
                DevReadBlock(indirect[i], blkPtr2);
            else
                break;
            DirEntry *dirPtr = (DirEntry *) blkPtr2;

            //compare paresed name with DirEntry's name
            for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
                printf("www...[%d][%d].name = '%s'\n", i, j, dirPtr[j].name);
                if (m == NULL) {
                    //if file name is ""
                    if (strcmp(dirPtr[j].name, "") == 0) {
                        //printf("vvv...indirect[%d], dirPtr[%d].name = '%s'\n",i, j, dirPtr[j].name);
                        //set retDirPtr, retDirPtrIndex, retBlkIndex
                        copyDirEnt(retDirPtr, dirPtr);
                        *retDirPtrIndex = j;
                        *retBlkIndex = indirect[i];
                        *is_full = false;
                        //free
                        free(root);
                        free(blkPtr);
                        free(blkPtr2);
                        return true;
                    } else if (j == NUM_OF_DIRENT_PER_BLOCK - 1)
                        *is_full = true;
                } else {
                    //printf("uuu\n");
                    //if file is already exist
                    if (strcmp(dirPtr[j].name, m->array[depth]) == 0) {
                        //set retDirPtr, retDirPtrIndex
                        *inoIndex = dirPtr[j].inodeNum;
                        copyDirEnt(retDirPtr, dirPtr);
                        *retDirPtrIndex = j;
                        *retBlkIndex = indirect[i];
                        //free
                        free(root);
                        free(blkPtr);
                        free(blkPtr2);
                        return true;
                    }
                }
            }
        }
    }
    printf("findName end...\n");
    //if not found, free
    free(root);
    free(blkPtr);
    free(blkPtr2);
    return false;
}

bool myMakeDir(Matrix *m, int depth, DirEntry *pDirPtr, int pDirPtrIndex, int pInoIndex, int pBlkIndex, bool is_full) {

    //if pDirPtr is full
    if (is_full) {
        //get parent inoPtr
        Inode *pInoPtr = (Inode *) malloc(sizeof(Inode));
        GetInode(pInoIndex, pInoPtr);

        if (pInoPtr->dirBlockPtr[1] == 0) {
            //init pDirPtr, pDirPtrIndex
            memset(pDirPtr, 0, BLOCK_SIZE);
            pDirPtrIndex = 0;
            //link dirPtr with parent inoPtr, write to disk, modify pBlkIndex
            int pFreeBlkIndex = GetFreeBlockNum();
            pInoPtr->dirBlockPtr[1] = pFreeBlkIndex;
            PutInode(pInoIndex, pInoPtr);
            pBlkIndex = pFreeBlkIndex;
            //set bitmap
            SetBlockBitmap(pFreeBlkIndex);
            //update file sys info
            DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
            pFileSysInfo->numAllocBlocks++;
            pFileSysInfo->numFreeBlocks--;
            DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
            //printf("myMakeDir()... if full...pInoIndex = %d, pFreeBlkIndex = %d...\n", pInoIndex, pFreeBlkIndex);
        } else {
            if (pInoPtr->indirBlockPtr == 0) {
                //init pDirPtr, pDirPtrIndex
                memset(pDirPtr, 0, BLOCK_SIZE);
                pDirPtrIndex = 0;
                //link indirPtr with parent inoPtr
                int pFreeBlkIndex = GetFreeBlockNum();
                pInoPtr->indirBlockPtr = pFreeBlkIndex;
                PutInode(pInoIndex, pInoPtr);
                //set bitmap
                SetBlockBitmap(pFreeBlkIndex);
                //update file sys info for indirPtr
                DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                pFileSysInfo->numAllocBlocks++;
                pFileSysInfo->numFreeBlocks--;
                DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                printf("myMakeDir()... if full...pInoIndex = %d, pInoPtr->indirBlockPtr = %d, pFreeBlkIndex = %d...\n",
                       pInoIndex, pInoPtr->indirBlockPtr, pFreeBlkIndex);

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
                printf("myMakeDir()... if full... pInoIndex = %d, pFreeBlkIndex2 = %d, pIndirPtr[0] = %d...\n",
                       pInoIndex, pFreeBlkIndex2, pIndirPtr[0]);

                //modify pBlkIndex
                pBlkIndex = pFreeBlkIndex2;
                //free
                free(pIndirBlkPtr);
            } else {
                //get indirPtr, link dirPtr with parent inoPtr, write to disk, modify pBlkIndex
                char *tmpBlk = (char *) malloc(BLOCK_SIZE);
                DevReadBlock(pInoPtr->indirBlockPtr, tmpBlk);
                int *indirPtr = (int *) tmpBlk;

                for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
                    if (indirPtr[i] == 0) {
                        //init pDirPtr, pDirPtrIndex
                        memset(pDirPtr, 0, BLOCK_SIZE);
                        pDirPtrIndex = 0;
                        int pFreeBlkIndex = GetFreeBlockNum();
                        indirPtr[i] = pFreeBlkIndex;
                        DevWriteBlock(pInoPtr->indirBlockPtr, (char *) indirPtr);
                        pBlkIndex = pFreeBlkIndex;
                        //set bitmap
                        SetBlockBitmap(pFreeBlkIndex);
                        //update file sys info
                        DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                        pFileSysInfo->numAllocBlocks++;
                        pFileSysInfo->numFreeBlocks--;
                        DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                        printf("myMakeDir()... if full & indirect != 0... indirPtr[%d] = %d, pBlkIndex = %d\n", i,
                               indirPtr[i], pBlkIndex);
                    }
                }
                free(tmpBlk);
            }
        }
        //free
        free(pInoPtr);
    }

    //get free inode index, block index
    int cFreeInoIndex = GetFreeInodeNum();
    int cFreeBlkIndex = GetFreeBlockNum();
    //printf("cFreeInoIndex = %d, cFreeBlkIndex = %d...\n", cFreeInoIndex, cFreeBlkIndex);

    //set parent dirPtr, write to disk
    strcpy(pDirPtr[pDirPtrIndex].name, m->array[depth]);
    pDirPtr[pDirPtrIndex].inodeNum = cFreeInoIndex;
    DevWriteBlock(pBlkIndex, (char *) pDirPtr);
    printf("myMakeDir()... pBlkIndex = %d, pDirPtr[%d].name = %s...\n", pBlkIndex, pDirPtrIndex,
           pDirPtr[pDirPtrIndex].name);
    //make child dirBlk, write to disk
    char *cBlkPtr = (char *) malloc(BLOCK_SIZE);
    memset(cBlkPtr, 0, BLOCK_SIZE);
    DirEntry *cDirPtr = (DirEntry *) cBlkPtr;
    strcpy(cDirPtr[0].name, ".");
    cDirPtr[0].inodeNum = cFreeInoIndex;
    strcpy(cDirPtr[1].name, "..");
    cDirPtr[1].inodeNum = pInoIndex;
    DevWriteBlock(cFreeBlkIndex, (char *) cDirPtr);
    //printf("cDirPtr[0].inodeNum = %d, cDirPtr[1].inodeNum = %d...\n", cDirPtr[0].inodeNum, cDirPtr[1].inodeNum);

    //set child inoPtr, write to disk
    Inode *cInoPtr = (Inode *) malloc(sizeof(Inode));
    memset(cInoPtr, 0, sizeof(Inode));
    GetInode(cFreeInoIndex, cInoPtr);
    cInoPtr->type = FILE_TYPE_DIR;
    cInoPtr->size = 0;
    cInoPtr->dirBlockPtr[0] = cFreeBlkIndex;
    cInoPtr->dirBlockPtr[1] = 0;
    cInoPtr->indirBlockPtr = 0;
    PutInode(cFreeInoIndex, cInoPtr);
    //printf("cInoPtr->dirBlockPtr[0] = %d, [1] = %d, indirec = %d...\n", cInoPtr->dirBlockPtr[0], cInoPtr->dirBlockPtr[1], cInoPtr->indirBlockPtr);

    //set bitmap
    SetInodeBitmap(cFreeInoIndex);
    SetBlockBitmap(cFreeBlkIndex);

    //get & update file sys info, write to disk
    DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
    pFileSysInfo->numAllocInodes++;
    pFileSysInfo->numAllocBlocks++;
    pFileSysInfo->numFreeBlocks--;
    DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);

    //free  malloc
    free(cBlkPtr);
    free(cInoPtr);
    return true;
}


int OpenFile(const char *szFileName, OpenFlag flag) {

}

int WriteFile(int fileDesc, char *pBuffer, int length) {
}

int ReadFile(int fileDesc, char *pBuffer, int length) {
}

int CloseFile(int fileDesc) {
}

int RemoveFile(const char *szFileName) {
}


int MakeDir(const char *szDirName) {
    int inoIndex = 0;

    //for save string
    Matrix m;
    initMatrix(&m, 2);

    //split path
    char *path = (char *) malloc(sizeof(szDirName));
    strcpy(path, szDirName);
    char *parsePtr = strtok(path, "/");
    //parse, save string
    while (parsePtr != NULL) {
        insertMatrix(&m, parsePtr);
        parsePtr = strtok(NULL, "/");
    }
    free(path);

    int depth = 0;
    DirEntry *retDirPtr = (DirEntry *) malloc(BLOCK_SIZE);
    int retDirPtrIndex = 0;
    int retBlkIndex = 19;
    bool is_full = false;
    //find
    while (findName(&inoIndex, &m, depth, retDirPtr, &retDirPtrIndex, &retBlkIndex, &is_full)) {
        inoIndex = retDirPtr[retDirPtrIndex].inodeNum;
        depth++;
    }
    //make dir
    if (m.array[depth] != NULL) {
        findName(&inoIndex, NULL, 0, retDirPtr, &retDirPtrIndex, &retBlkIndex, &is_full);
        if (myMakeDir(&m, depth, retDirPtr, retDirPtrIndex, inoIndex, retBlkIndex, is_full)) {
            freeMatrix(&m);
            return 0;
        } else {
            freeMatrix(&m);
            return -1;
        }
    } else {
        freeMatrix(&m);
        return -1;
    }
}

bool myRemoveDir(Matrix *m, int depth, DirEntry *pDirPtr, int pDirPtrIndex, int pInoIndex, int pBlkIndex) {
    //get parent inode, child inode
    Inode *pInoPtr = (Inode *) malloc(sizeof(Inode));
    GetInode(pInoIndex, pInoPtr);
    Inode *cInoPtr = (Inode *) malloc(sizeof(Inode));
    int cInoIndex = pDirPtr[pDirPtrIndex].inodeNum;
    GetInode(cInoIndex, cInoPtr);

    //get tmpBlk
    char *tmpBlk = (char *) malloc(BLOCK_SIZE);
    char *tmpBlk2 = (char *) malloc(BLOCK_SIZE);

    bool empty = true;
    //check empty dir at direct
    for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {
        //get dirBlk, cast to DirEntry*
        if (cInoPtr->dirBlockPtr != 0)
            DevReadBlock(cInoPtr->dirBlockPtr[i], tmpBlk);
        else
            continue;
        DirEntry *dirPtr = (DirEntry *) tmpBlk;

        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            //if found corresponding dir name
            if (strcmp(dirPtr[j].name, ".") == 0 || strcmp(dirPtr[j].name, "..") == 0)
                continue;
            if (strcmp(dirPtr[j].name, "") != 0)
                empty = false;
        }
    }
    //at indirect
    if (cInoPtr->indirBlockPtr != 0 && empty) {
        //get indirPtr, cast to int*
        DevReadBlock(cInoPtr->indirBlockPtr, tmpBlk);
        int *indirPtr = (int *) tmpBlk;

        for (int k = 0; k < BLOCK_SIZE / sizeof(int); k++) {
            //if not empty dir block
            if (indirPtr[k] != 0) {
                //get block, cast to DirEntry*

                DevReadBlock(indirPtr[k], tmpBlk2);
                DirEntry *dirPtr = (DirEntry *) tmpBlk2;

                for (int l = 0; l < NUM_OF_DIRENT_PER_BLOCK; l++) {
                    if (strcmp(dirPtr[l].name, "") != 0)
                        empty = false;
                }
            }
                //if empty dir block
            else {
                continue;
            }
        }
    }
    //if empty dir
    if (empty) {
        //reset dirEntry block
        memset(tmpBlk, 0, BLOCK_SIZE);
        DevWriteBlock(cInoPtr->dirBlockPtr[0], tmpBlk);

        //reset child inode
        memset(cInoPtr, 0, sizeof(Inode));
        PutInode(pDirPtr[pDirPtrIndex].inodeNum, cInoPtr);

        //reset bitmap
        ResetBlockBitmap(cInoPtr->dirBlockPtr[0]);
        ResetInodeBitmap(pDirPtr[pDirPtrIndex].inodeNum);

        //update fs info
        if (pFileSysInfo == NULL) {
            pFileSysInfo = (FileSysInfo *) malloc(BLOCK_SIZE);
            DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
        }
        pFileSysInfo->numAllocBlocks--;
        pFileSysInfo->numFreeBlocks++;
        pFileSysInfo->numAllocInodes--;

        //Modify parent dirEntry, write to disk
        strcpy(pDirPtr[pDirPtrIndex].name, "");
        pDirPtr[pDirPtrIndex].inodeNum = 0;
        DevWriteBlock(pBlkIndex, (char *) pDirPtr);

        //modify parent inode
        //check if emtpy dirEntryPtr
        bool empty = true;
        for (int i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++) {
            if (strcmp(pDirPtr[i].name, "") != 0)
                empty = false;
        }
        //optimize dirEntry
        if (!empty) {
            for (int i = NUM_OF_DIRENT_PER_BLOCK - 1; i >= 0; i--) {
                //if found not empty dirEntry, move to empty dirEntry
                if (strcmp(pDirPtr[i].name, "") != 0) {
                    strcpy(pDirPtr[pDirPtrIndex].name, pDirPtr[i].name);
                    pDirPtr[pDirPtrIndex].inodeNum = pDirPtr[i].inodeNum;
                    strcmp(pDirPtr[i].name, "");
                    pDirPtr[i].inodeNum = 0;
                    DevWriteBlock(pBlkIndex, (char *) pDirPtr);
                    break;
                }
            }
        }
        //if empty dirEntryPtr
        if (empty) {
            //at direct
            if (pInoPtr->dirBlockPtr[1] == pBlkIndex) {
                //unlink, reset data block
                pInoPtr->dirBlockPtr[1] = 0;
                memset(tmpBlk, 0, BLOCK_SIZE);
                DevWriteBlock(pBlkIndex, tmpBlk);
                pFileSysInfo->numAllocBlocks--;
                pFileSysInfo->numFreeBlocks++;
            }

            //at indirect
            if (pInoPtr->indirBlockPtr != 0) {
                //get indir block, cast to int*
                DevReadBlock(pInoPtr->indirBlockPtr, tmpBlk);
                int *indirPtr = (int *) tmpBlk;
                int indirPtrIndex = 0;

                for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
                    //if found empty block at indirect, reset data block
                    if (indirPtr[i] == pBlkIndex) {
                        memset(tmpBlk, 0, BLOCK_SIZE);
                        DevWriteBlock(pBlkIndex, tmpBlk);
                        pFileSysInfo->numAllocBlocks--;
                        pFileSysInfo->numFreeBlocks++;
                        indirPtr[i] = 0;
                        indirPtrIndex = i;
                    }
                    if (indirPtr[i] != 0)
                        empty = false;
                }
                //optimize indirPtr
                if (!empty) {
                    for (int i = BLOCK_SIZE / sizeof(int) - 1; i >= 0; i--) {
                        if (indirPtr[i] != 0) {
                            indirPtr[indirPtrIndex] = i;
                            indirPtr[i] = 0;
                            DevWriteBlock(pInoPtr->indirBlockPtr, tmpBlk2);
                        }
                    }
                }
                //if all indirPtr is empty
                if (empty) {
                    //unlink, reset block data
                    pInoPtr->indirBlockPtr = 0;
                    DevWriteBlock(pInoPtr->indirBlockPtr, tmpBlk);
                    pFileSysInfo->numAllocBlocks--;
                    pFileSysInfo->numFreeBlocks++;
                }
            }
        }

        //update fsInfo
        DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
    } else {
        //free
        free(pInoPtr);
        free(cInoPtr);
        free(tmpBlk);
        free(tmpBlk2);
        //if not empty dir

        return -1;
    }
}

int RemoveDir(const char *szDirName) {
    int inoIndex = 0;

    //for save string
    Matrix m;
    initMatrix(&m, 2);
    //split path
    char *path = (char *) malloc(sizeof(szDirName));
    strcpy(path, szDirName);
    char *parsePtr = strtok(path, "/");
    //parse, save string
    while (parsePtr != NULL) {
        insertMatrix(&m, parsePtr);
        parsePtr = strtok(NULL, "/");
    }
    free(path);
    int depth = 0;
    DirEntry *retDirPtr = (DirEntry *) malloc(BLOCK_SIZE);
    int retDirPtrIndex = 0;
    int retBlkIndex = 19;
    bool is_full = false;
    //find
    while (findName(&inoIndex, &m, depth, retDirPtr, &retDirPtrIndex, &retBlkIndex, &is_full)) {
        //if found dir name to remove
        if (m.array[depth + 1] == NULL) {
            //remove dir
            if (myRemoveDir(&m, depth, retDirPtr, retDirPtrIndex, inoIndex, retBlkIndex))
                return 0;
        }
        inoIndex = retDirPtr[retDirPtrIndex].inodeNum;
        depth++;
    }
    //if not found
    return -1;
}

int EnumerateDirStatus(const char *szDirName, DirEntryInfo *pDirEntry, int dirEntrys) {
    int inoIndex = 0;
    int count = 0;

    //for save string
    Matrix m;
    initMatrix(&m, 2);
    //split path
    char *path = (char *) malloc(sizeof(szDirName));
    strcpy(path, szDirName);
    char *parsePtr = strtok(path, "/");
    //parse, save string
    while (parsePtr != NULL) {
        insertMatrix(&m, parsePtr);
        parsePtr = strtok(NULL, "/");
    }
    free(path);

    int depth = 0;
    DirEntry *retDirPtr = (DirEntry *) malloc(BLOCK_SIZE);
    int retDirPtrIndex = 0;
    int retBlkIndex = 19;
    bool is_full = false;
    //find
    while (findName(&inoIndex, &m, depth, retDirPtr, &retDirPtrIndex, &retBlkIndex, &is_full)) {
        //if found dir name to make
        if (m.array[depth + 1] == NULL) {
            //get inode
            Inode *inoPtr = (Inode *) malloc(sizeof(Inode));
            Inode *cInoPtr = (Inode *) malloc(sizeof(Inode));
            GetInode(retDirPtr[retDirPtrIndex].inodeNum, inoPtr);
            char *tmpBlk = (char *) malloc(BLOCK_SIZE);
            char *tmpBlk2 = (char *) malloc(BLOCK_SIZE);

            //get DirEntryInfo at direct
            for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {
                if (inoPtr->dirBlockPtr[i] != 0) {
                    //get dir block, cast to dirEntry*
                    DevReadBlock(inoPtr->dirBlockPtr[i], tmpBlk);
                    DirEntry *dirPtr = (DirEntry *) tmpBlk;

                    //for all dir entry
                    for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
                        if (strcmp(dirPtr[j].name, "") != 0) {
                            //get child inode to get type
                            GetInode(dirPtr[j].inodeNum, cInoPtr);
                            pDirEntry[count].type = cInoPtr->type;
                            strcpy(pDirEntry[count].name, dirPtr[j].name);
                            pDirEntry[count].inodeNum = dirPtr[j].inodeNum;
                            count++;
                        }
                    }
                }
            }
            //get DirEntryInfo at indirect
            if (inoPtr->indirBlockPtr != 0) {
                //get indir block, cast to DirEntry*
                DevReadBlock(inoPtr->indirBlockPtr, tmpBlk);
                int *indirPtr = (int *) tmpBlk;
                for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
                    if (indirPtr[i] != 0) {
                        //get data block, cast to DirEntry*
                        DevReadBlock(indirPtr[i], tmpBlk2);
                        DirEntry *dirPtr = (DirEntry *) tmpBlk2;

                        //for all dir entry
                        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
                            if (strcmp(dirPtr[j].name, "") != 0) {
                                //get child inode to get type
                                GetInode(dirPtr[j].inodeNum, cInoPtr);
                                pDirEntry[count].type = cInoPtr->type;
                                strcpy(pDirEntry[count].name, dirPtr[j].name);
                                pDirEntry[count].inodeNum = dirPtr[j].inodeNum;
                                count++;
                            }
                        }
                    }
                }
            }
            if (retDirPtr != NULL)
                free(retDirPtr);
            if (inoPtr != NULL)
                free(inoPtr);
            if (cInoPtr != NULL)
                free(cInoPtr);
            if (tmpBlk != NULL)
                free(tmpBlk);
            if (tmpBlk2 != NULL)
                free(tmpBlk2);
            return count;
        }
        inoIndex = retDirPtr[retDirPtrIndex].inodeNum;
        depth++;
    }
    if (retDirPtr != NULL)
        free(retDirPtr);
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
    int blkininode = BLOCK_SIZE / sizeof(Inode);
    if (inodeno >= INODELIST_BLOCKS * blkininode)
        return;
    int blk_posi = INODELIST_BLOCK_FIRST + inodeno / blkininode;
    int inode_posi = inodeno % blkininode;
    char *inodeblk = malloc(BLOCK_SIZE);

    DevReadBlock(blk_posi, inodeblk);
    memcpy(inodeblk + (inode_posi * sizeof(Inode)), pInode, sizeof(Inode));
    DevWriteBlock(blk_posi, inodeblk);

    free(inodeblk);
    //
    //
    //    char *buf = malloc(BLOCK_SIZE);
    //    //DevOpenDisk();
    //    DevReadBlock((inodeno / 8) + INODELIST_BLOCK_FIRST, buf);
    //    memcpy(buf + (inodeno % 8) * sizeof(Inode), pInode, sizeof(Inode));
    //    DevWriteBlock((inodeno / 8) + INODELIST_BLOCK_FIRST, buf);
    //    free(buf);
}

void GetInode(int inodeno, Inode *pInode) {

    //DevOpenDisk();
    int blkininode = BLOCK_SIZE / sizeof(Inode);
    if (inodeno >= INODELIST_BLOCKS * blkininode)
        return;
    int blk_posi = INODELIST_BLOCK_FIRST + inodeno / blkininode;
    int inode_posi = inodeno % blkininode;
    char *inodeblk = (char *) malloc(BLOCK_SIZE);

    DevReadBlock(blk_posi, inodeblk);
    memcpy(pInode, inodeblk + (inode_posi * sizeof(Inode)), sizeof(Inode));

    free(inodeblk);
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
