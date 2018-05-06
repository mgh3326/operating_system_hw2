//-fs.c: 과제1에서 구현한 코드를 이 파일에 넣고, 빈 함수를 구현하면 됩니다.
#include <stdio.h>
#include <stdlib.h>
#include "Disk.h"
#include "fs.h"
#include <string.h>

FileDescTable *pFileDescTable = NULL;

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
    DirEntry *pDirEntry = NULL;
    pDirEntry = malloc(sizeof *pDirEntry); // 이렇게 할당 malloc 해주면 되는건가

    Inode *pInode = NULL;
    pInode = malloc(sizeof(pInode));
    //    strncpy(pDirEntry->name, szDirName,12);
    char arr[64][MAX_NAME_LEN + 1]; //폴더 갯수 최대 그냥 64로 했음
    int index = 0;
    //형준쓰거
    char *path = (char *) malloc(sizeof(szDirName));
    strcpy(path, szDirName);
    char *parsePtr = strtok(path, "/");

    while (parsePtr != NULL) {
        //insertMatrix(&m, parsePtr);
        //printf("%s",parsePtr);
        strcpy(arr[index++], parsePtr);
        parsePtr = strtok(NULL, "/");
    }
    for(int i=0;i<index;i++)
    {
        printf("%s\n",arr[i]);
    }
    strncpy(pDirEntry->name, szDirName, sizeof(pDirEntry->name) - 1); //strcpy는 안좋다니까 strncpy로 함
    //    pDirEntry->inodeNum = GetFreeInodeNum();
    //    pInode->size = 1; //이게 바이트 크기인가
    //    pInode->type = FILE_TYPE_DIR;
    //
    //    SetInodeBitmap(pDirEntry->inodeNum);
    //    PutInode(pDirEntry->inodeNum, pInode); //pInode 내용을 넣어주어야겠다.
    return 0;
}

int RemoveDir(const char *szDirName) {
}

void EnumerateDirStatus(const char *szDirName, DirEntry *pDirEntry, int *pNum) {
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
    memset(buf, 0, BLOCK_SIZE);   // memset을 통해서 모든 메모리를 0으로 만듭니다.
    for (int i = 0; i < 512; i++) //512 블록까지 초기화 하라는건가?
    {
        DevWriteBlock(i, buf);
    }
    free(buf);
}

void SetInodeBitmap(int inodeno) {
    char *buf = malloc(BLOCK_SIZE);
    //DevOpenDisk();
    DevReadBlock(INODE_BITMAP_BLK_NUM, buf);
    buf[inodeno / 8] |= 1 << (7 - (inodeno % 8)); // if(inodeno % 8 ==0) -> buf[inodeno/8] |= 1000000
    DevWriteBlock(INODE_BITMAP_BLK_NUM, buf);
    free(buf);
}

void ResetInodeBitmap(int inodeno) {
    char *buf = malloc(BLOCK_SIZE);
    //DevOpenDisk();
    DevReadBlock(INODE_BITMAP_BLK_NUM, buf);
    buf[inodeno / 8] &= ~(1 << (7 - (inodeno % 8)));
    DevWriteBlock(INODE_BITMAP_BLK_NUM, buf);
    free(buf);
}

void SetBlockBitmap(int blkno) {

    char *buf = malloc(BLOCK_SIZE);
    //DevOpenDisk();
    DevReadBlock(BLOCK_BITMAP_BLK_NUM, buf);
    buf[blkno / 8] |= 1 << (7 - (blkno % 8));
    DevWriteBlock(BLOCK_BITMAP_BLK_NUM, buf);
    free(buf);
}

void ResetBlockBitmap(int blkno) {
    char *buf = malloc(BLOCK_SIZE);
    //DevOpenDisk();
    DevReadBlock(BLOCK_BITMAP_BLK_NUM, buf);
    buf[blkno / 8] &= ~(1 << (7 - (blkno % 8)));
    DevWriteBlock(BLOCK_BITMAP_BLK_NUM, buf);
    free(buf);
}

void PutInode(int inodeno, Inode *pInode) {

    char *buf = malloc(BLOCK_SIZE);
    //DevOpenDisk();
    DevReadBlock((inodeno / 8) + INODELIST_BLK_FIRST, buf);
    memcpy(buf + (inodeno % 8) * sizeof(Inode), pInode, sizeof(Inode));
    DevWriteBlock((inodeno / 8) + INODELIST_BLK_FIRST, buf);
    free(buf);
}

void GetInode(int inodeno, Inode *pInode) {

    char *buf = malloc(BLOCK_SIZE);
    //DevOpenDisk();
    DevReadBlock((inodeno / 8) + INODELIST_BLK_FIRST, buf);
    memcpy(pInode, buf + (inodeno % 8) * sizeof(Inode), sizeof(Inode));
    free(buf);
}

int GetFreeInodeNum(void) {
    char *buf = malloc(BLOCK_SIZE);
    ////DevOpenDisk();
    DevReadBlock(INODE_BITMAP_BLK_NUM, buf);

    for (int i = 0; i < BLOCK_SIZE; i++) {
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
    DevReadBlock(BLOCK_BITMAP_BLK_NUM, buf);

    for (int i = 0; i < BLOCK_SIZE; i++) {
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
