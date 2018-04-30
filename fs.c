#include <stdio.h>
#include <stdlib.h>
#include "Disk.h"
#include "fs.h"
#include <stdlib.h>
#include <string.h>

FileDescTable *pFileDescTable = NULL;

int OpenFile(const char *szFileName, OpenFlag flag)
{
}

int WriteFile(int fileDesc, char *pBuffer, int length)
{
}

int ReadFile(int fileDesc, char *pBuffer, int length)
{
}

int CloseFile(int fileDesc)
{
}

int RemoveFile(const char *szFileName)
{
}

int MakeDir(const char *szDirName)
{
}

int RemoveDir(const char *szDirName)
{
}

void EnumerateDirStatus(const char *szDirName, DirEntry *pDirEntry, int *pNum)
{
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
    for (int i = 0; i < 7; i++) //1부터 6까지라서
    {
        DevWriteBlock(i, buf);
    }
    free(buf);
}
void SetInodeBitmap(int inodeno)
{
    char *buf = malloc(BLOCK_SIZE);
    //DevOpenDisk();
    DevReadBlock(INODE_BITMAP_BLK_NUM, buf);
    buf[inodeno / 8] |= 1 << (7 - (inodeno % 8)); // if(inodeno % 8 ==0) -> buf[inodeno/8] |= 1000000
    DevWriteBlock(INODE_BITMAP_BLK_NUM, buf);
    free(buf);
}

void ResetInodeBitmap(int inodeno)
{
    char *buf = malloc(BLOCK_SIZE);
    //DevOpenDisk();
    DevReadBlock(INODE_BITMAP_BLK_NUM, buf);
    buf[inodeno / 8] &= ~(1 << (7 - (inodeno % 8)));
    DevWriteBlock(INODE_BITMAP_BLK_NUM, buf);
    free(buf);
}

void SetBlockBitmap(int blkno)
{

    char *buf = malloc(BLOCK_SIZE);
    //DevOpenDisk();
    DevReadBlock(BLOCK_BITMAP_BLK_NUM, buf);
    buf[blkno / 8] |= 1 << (7 - (blkno % 8));
    DevWriteBlock(BLOCK_BITMAP_BLK_NUM, buf);
    free(buf);
}

void ResetBlockBitmap(int blkno)
{
    char *buf = malloc(BLOCK_SIZE);
    //DevOpenDisk();
    DevReadBlock(BLOCK_BITMAP_BLK_NUM, buf);
    buf[blkno / 8] &= ~(1 << (7 - (blkno % 8)));
    DevWriteBlock(BLOCK_BITMAP_BLK_NUM, buf);
    free(buf);
}

void PutInode(int inodeno, Inode *pInode)
{

    char *buf = malloc(BLOCK_SIZE);
    //DevOpenDisk();
    DevReadBlock((inodeno / 8) + INODELIST_BLK_FIRST, buf);
    memcpy(buf + (inodeno % 8) * sizeof(Inode), pInode, sizeof(Inode));
    DevWriteBlock((inodeno / 8) + INODELIST_BLK_FIRST, buf);
    free(buf);
}

void GetInode(int inodeno, Inode *pInode)
{

    char *buf = malloc(BLOCK_SIZE);
    //DevOpenDisk();
    DevReadBlock((inodeno / 8) + INODELIST_BLK_FIRST, buf);
    memcpy(pInode, buf + (inodeno % 8) * sizeof(Inode), sizeof(Inode));
    free(buf);
}

int GetFreeInodeNum(void)
{
    char *buf = malloc(BLOCK_SIZE);
    ////DevOpenDisk();
    DevReadBlock(INODE_BITMAP_BLK_NUM, buf);

    for (int i = 0; i < BLOCK_SIZE; i++)
    {
        if (buf[i] == -1) // if 11111111 => continue
            continue;
        for (int j = 0; j < 8; j++)
        {
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

int GetFreeBlockNum(void)
{
    char *buf = malloc(BLOCK_SIZE);
    DevReadBlock(BLOCK_BITMAP_BLK_NUM, buf);

    for (int i = 0; i < BLOCK_SIZE; i++)
    {
        if (buf[i] == -1) // if 11111111 => continue
            continue;

        for (int j = 0; j < 8; j++)
        {
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
