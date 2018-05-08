//-fs.c: 과제1에서 구현한 코드를 이 파일에 넣고, 빈 함수를 구현하면 됩니다.
#include <stdio.h>
#include <stdlib.h>
#include "disk.h"
#include "fs.h"
#include <string.h>

FileDescTable *pFileDescTable = NULL;

int FindDir(int *inoIndex, char arr[64][MAX_NAME_LEN + 1], int arr_index, DirEntry *retDirPtr, int *retDirPtrIndex,
            int *retBlkIndex) //폴더 찾기
{
    //root의 tmp 찾기
    Inode *root = (Inode *)malloc(sizeof(Inode));
    GetInode(*inoIndex, root);

    //alloc memory
    char *blkPtr = (char *)malloc(BLOCK_SIZE);

    for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++)
    {
        if (root->dirBlockPtr[i] != 0)
            DevReadBlock(root->dirBlockPtr[i], blkPtr);
        else
            break;
        DirEntry *dirPtr = (DirEntry *)blkPtr;
        for (int i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++)
        {
            if (strcmp(dirPtr[i].name, arr[arr_index - 1]) == 0)
            {
                //set retDirPtr, retDirPtrIndex
                *inoIndex = root->dirBlockPtr[i];
                //						copyDirEnt(retDirPtr, dirPtr);
                *retDirPtrIndex = i;

                //free
                free(root);
                free(blkPtr);

                return 0;
            }
        }
        printf("ttt\n");
    }
}

int Findinode(const char *szFileName, int _inode) //retrun inode
{
    printf("Findinode filename : %s\n", szFileName);
    Inode *root = (Inode *)malloc(sizeof(Inode));
    GetInode(_inode, root); // 루트여서 0번째 inode 확인
    char *blkPtr = (char *)malloc(BLOCK_SIZE);
    DevReadBlock(root->dirBlockPtr[0], blkPtr); //이거 일단 0번째 읽게함

    DirEntry *dirPtr = (DirEntry *)blkPtr;
    for (int j = 0; j < NUM_OF_DIRECT_BLOCK_PTR; j++)
    {
        for (int i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++)
        { //이건 4번이겠네
            if (strcmp(dirPtr[i].name, szFileName) == 0)
            {

                //free
                int temp = dirPtr[i].inodeNum;
                free(root);
                free(blkPtr);
                // printf("temp 리턴\n");
                return temp;
            }
        }
    }

    printf("Indirect node 필요");
    return -1;
}

int OpenFile(const char *szFileName, OpenFlag flag)
{
    if (flag == OPEN_FLAG_CREATE)
    {
        char arr[64][MAX_NAME_LEN + 1]; //폴더 갯수 최대 그냥 64로 했음
        int arr_index = 0;

        char *path = (char *)malloc(sizeof(szFileName));
        strcpy(path, szFileName);
        char *parsePtr = strtok(path, "/");

        while (parsePtr != NULL)
        {
            strcpy(arr[arr_index++], parsePtr);
            parsePtr = strtok(NULL, "/");
        }
        int *inoIndex = (int *)malloc(sizeof(int));
        *inoIndex = 0;

        int *retDirPtrIndex = (int *)malloc(sizeof(int));
        *retDirPtrIndex = 0;
        int *retBlkIndex = (int *)malloc(sizeof(int));
        *retBlkIndex = 19;
        //        DirEntry *pDirEntry[4]; // 크기가 4인 구조체 포인터 배열 선언
        //        // 구조체 포인터 배열 전체 크기에서 요소(구조체 포인터)의 크기로 나눠서 요소 개수를 구함
        //        for (int i = 0; i < sizeof(pDirEntry) / sizeof(DirEntry *); i++) // 요소 개수만큼 반복
        //        {
        //            pDirEntry[i] = malloc(sizeof(DirEntry)); // 각 요소에 구조체 크기만큼 메모리 할당
        //        }
        DirEntry *pDirEntry = (DirEntry *)malloc(BLOCK_SIZE);

        int temp = 0;
        for (int i = 0; i < arr_index - 1; i++)
        { //마지막 인덱스는 빼자
            temp = Findinode(arr[i], temp);
            // printf("temp : %d  getfreeinodenum : %d\n", temp, GetFreeInodeNum());
        }
        int pinode = GetFreeInodeNum();
        Inode *inoPtr = (Inode *)malloc(sizeof(Inode));

        GetInode(pinode, inoPtr);
        inoPtr->size = 0;
        inoPtr->type = FILE_TYPE_FILE;
        PutInode(pinode, inoPtr);
        SetInodeBitmap(pinode);
        char *cBlkPtr = (char *)malloc(BLOCK_SIZE);
        DevReadBlock(FILESYS_INFO_BLOCK, cBlkPtr);
        FileSysInfo *sysPtr = (FileSysInfo *)cBlkPtr;
        sysPtr->numAllocInodes++;
        DevWriteBlock(FILESYS_INFO_BLOCK, cBlkPtr);
        if (pFileDescTable == NULL)
        {
            //할당
            pFileDescTable = (FileDescTable *)malloc(sizeof(FileDescTable));
            //0으로 초기화
            memset(pFileDescTable, 0, sizeof(FileDescTable));
        }
        FileDesc *fdPtr = (FileDesc *)pFileDescTable;
        int fFdIndex = 0;
        for (int i = 0; i < MAX_FD_ENTRY_LEN; i++)
        {
            //if find free fd index
            if (fdPtr[i].bUsed == 0)
            {
                fFdIndex = i;
                fdPtr[i].bUsed = 1;
                fdPtr[i].fileOffset = 0;
                fdPtr[i].inodeNum = pinode;
                break;
            }
        }
        free(cBlkPtr);
        free(inoPtr);
        //fFdIndex 이걸 활용해야하네
        // FindDir(inoIndex, arr, arr_index, pDirEntry, retDirPtrIndex, retBlkIndex, 0);//처음이니까 0
        printf("oepnfile end\n");
        return fFdIndex;
    }
    else //이 경우도 있나
    {
        return 0;
    }
    return -1;
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

int OneMakeDir(const char *szDirName, DirEntry *pDirEntry, int pDirPtrIndex, int pInoIndex, int pBlkIndex) //인자 이게 맞나
{
    //여기부터 temp일 경우

    int inode_index = GetFreeInodeNum();
    int block_index = GetFreeBlockNum();
    // int pDirPtrIndex = 0;
    strncpy(pDirEntry[pDirPtrIndex].name, szDirName,
            sizeof(pDirEntry[pDirPtrIndex].name) - 1); //strcpy는 안좋다니까 strncpy로 함

    pDirEntry[pDirPtrIndex].inodeNum = inode_index;
    DevWriteBlock(pBlkIndex, (char *)pDirEntry);

    char *cBlkPtr = (char *)malloc(BLOCK_SIZE);
    DirEntry *cDirPtr = (DirEntry *)cBlkPtr;

    //set DirEntry's data, write to disk
    strcpy(cDirPtr[0].name, ".");
    cDirPtr[0].inodeNum = inode_index;
    strcpy(cDirPtr[1].name, "..");
    cDirPtr[1].inodeNum = pInoIndex;
    DevWriteBlock(block_index, cBlkPtr);

    Inode *pInode = NULL;
    pInode = malloc(sizeof(pInode));
    GetInode(inode_index, pInode);
    pInode->size = 0; //이게 바이트 크기인가
    pInode->type = FILE_TYPE_DIR;
    pInode->dirBlockPtr[0] = block_index;
    PutInode(inode_index, pInode);

    SetInodeBitmap(inode_index);
    SetBlockBitmap(block_index);

    //get sys info, write to disk
    DevReadBlock(FILESYS_INFO_BLOCK, cBlkPtr);
    FileSysInfo *sysPtr = (FileSysInfo *)cBlkPtr;
    sysPtr->numAllocBlocks++;
    sysPtr->numFreeBlocks--;
    sysPtr->numAllocInodes++;
    DevWriteBlock(FILESYS_INFO_BLOCK, cBlkPtr);

    //free
    //    free(cBlkPtr);
    //    free(pInode);
    return 0;
}

int MakeDir(const char *szDirName)
{
    // DirEntry *pDirEntry = NULL;
    // pDirEntry = malloc(sizeof *pDirEntry); // 이렇게 할당 malloc 해주면 되는건가

    //    strncpy(pDirEntry->name, szDirName,12);
    char arr[64][MAX_NAME_LEN + 1]; //폴더 갯수 최대 그냥 64로 했음
    int arr_index = 0;
    //ㅎㅈ쓰거
    char *path = (char *)malloc(sizeof(szDirName));
    strcpy(path, szDirName);
    char *parsePtr = strtok(path, "/");

    while (parsePtr != NULL)
    {
        strcpy(arr[arr_index++], parsePtr);
        parsePtr = strtok(NULL, "/");
    }
    int *inoIndex = (int *)malloc(sizeof(int));
    *inoIndex = 0;
    //    DirEntry *pDirEntry[4]; // 크기가 4인 구조체 포인터 배열 선언
    //    // 구조체 포인터 배열 전체 크기에서 요소(구조체 포인터)의 크기로 나눠서 요소 개수를 구함
    //    for (int i = 0; i < sizeof(pDirEntry) / sizeof(DirEntry *); i++) // 요소 개수만큼 반복
    //    {
    //        pDirEntry[i] = malloc(sizeof(DirEntry)); // 각 요소에 구조체 크기만큼 메모리 할당
    //    }
    DirEntry *pDirEntry = (DirEntry *)malloc(BLOCK_SIZE);

    // DirEntry *retDirPtr = (DirEntry *)malloc(BLOCK_SIZE);
    int *retDirPtrIndex = (int *)malloc(sizeof(int));
    *retDirPtrIndex = 1;
    int *retBlkIndex = (int *)malloc(sizeof(int));
    *retBlkIndex = 19;
    Inode *pInode = NULL;
    pInode = malloc(sizeof *pInode); // 이렇게 할당 malloc 해주면 되는건가

    GetInode(0, pInode); //이게 뭐지
    char *blkPtr = (char *)malloc(BLOCK_SIZE);
    DevReadBlock(pInode->dirBlockPtr[0], blkPtr);
    DirEntry *dirPtr = (DirEntry *)blkPtr;

    //19이걸 읽자
    OneMakeDir(arr[arr_index - 1], dirPtr, *retDirPtrIndex, *inoIndex, *retBlkIndex);

    printf("%s\n", arr[arr_index - 1]);

    return 0;
}

int RemoveDir(const char *szDirName)
{
}

int EnumerateDirStatus(const char *szDirName, DirEntryInfo *pDirEntry, int dirEntrys)
{
    printf("Enumer Start\n");
    char arr[64][MAX_NAME_LEN + 1]; //폴더 갯수 최대 그냥 64로 했음
    int arr_index = 0;

    char *path = (char *)malloc(sizeof(szDirName));
    strcpy(path, szDirName);
    char *parsePtr = strtok(path, "/");

    while (parsePtr != NULL)
    {
        strcpy(arr[arr_index++], parsePtr);
        parsePtr = strtok(NULL, "/");
    }
    return 0;
    return -1; //실패
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

void SetInodeBitmap(int inodeno)
{
    char *buf = malloc(BLOCK_SIZE);
    //DevOpenDisk();
    DevReadBlock(INODE_BITMAP_BLOCK_NUM, buf);
    buf[inodeno / 8] |= 1 << (7 - (inodeno % 8)); // if(inodeno % 8 ==0) -> buf[inodeno/8] |= 1000000
    DevWriteBlock(INODE_BITMAP_BLOCK_NUM, buf);
    free(buf);
}

void ResetInodeBitmap(int inodeno)
{
    char *buf = malloc(BLOCK_SIZE);
    //DevOpenDisk();
    DevReadBlock(INODE_BITMAP_BLOCK_NUM, buf);
    buf[inodeno / 8] &= ~(1 << (7 - (inodeno % 8)));
    DevWriteBlock(INODE_BITMAP_BLOCK_NUM, buf);
    free(buf);
}

void SetBlockBitmap(int blkno)
{

    char *buf = malloc(BLOCK_SIZE);
    //DevOpenDisk();
    DevReadBlock(BLOCK_BITMAP_BLOCK_NUM, buf);
    buf[blkno / 8] |= 1 << (7 - (blkno % 8));
    DevWriteBlock(BLOCK_BITMAP_BLOCK_NUM, buf);
    free(buf);
}

void ResetBlockBitmap(int blkno)
{
    char *buf = malloc(BLOCK_SIZE);
    //DevOpenDisk();
    DevReadBlock(BLOCK_BITMAP_BLOCK_NUM, buf);
    buf[blkno / 8] &= ~(1 << (7 - (blkno % 8)));
    DevWriteBlock(BLOCK_BITMAP_BLOCK_NUM, buf);
    free(buf);
}

void PutInode(int inodeno, Inode *pInode)
{

    char *buf = malloc(BLOCK_SIZE);
    //DevOpenDisk();
    DevReadBlock((inodeno / 8) + INODELIST_BLOCK_FIRST, buf);
    memcpy(buf + (inodeno % 8) * sizeof(Inode), pInode, sizeof(Inode));
    DevWriteBlock((inodeno / 8) + INODELIST_BLOCK_FIRST, buf);
    free(buf);
}

void GetInode(int inodeno, Inode *pInode)
{

    char *buf = malloc(BLOCK_SIZE);
    //DevOpenDisk();
    DevReadBlock((inodeno / 8) + INODELIST_BLOCK_FIRST, buf);
    memcpy(pInode, buf + (inodeno % 8) * sizeof(Inode), sizeof(Inode));
    free(buf);
}

int GetFreeInodeNum(void)
{
    char *buf = malloc(BLOCK_SIZE);
    ////DevOpenDisk();
    DevReadBlock(INODE_BITMAP_BLOCK_NUM, buf);

    for (int i = 0; i < BLOCK_SIZE; i++) //이러면 24부터네
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
    DevReadBlock(BLOCK_BITMAP_BLOCK_NUM, buf);
    int i = 2; // 19부터 하기 위함
    for (int j = 3; j < 8; j++)
    {
        if ((buf[i] << j & 128) == 0) // >> 연산자가 & 보다 우선순위가 높구나
        {
            free(buf);
            return (i * 8) + j;
        }
    }
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
