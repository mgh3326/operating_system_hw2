//-fs.c: 과제1에서 구현한 코드를 이 파일에 넣고, 빈 함수를 구현하면 됩니다.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "disk.h"
#include "fs.h"
#include <math.h>

FileDescTable *pFileDescTable = NULL;
#define START_INDEX_DATA_REGION 19
typedef enum _UPDATE_FLAG {
    ADD_DIR,
    ALOCATE_BLOCK,
    ALOCATE_INODE,
    FREE_BLOCK,
    FREE_INODE
} UPDATE_FLAG;


int addNewDirEntry(int index, Inode *parentInode, int parentInodeNum) {

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

int addNewDirBlock(int index, char *target, Inode *parentInode, int parentInodeNum, int *parentBlockNum,
                   DirEntry *parentDE) {

    int newBlockNum = -1;
    int curBlockNum = parentInode->dirBlockPtr[index];
    int i;
    for (i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++) {
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
            DevWriteBlock(parentBlockNum, parentDE);

            newBlockNum = GetFreeBlockNum();
            SetBlockBitmap(newBlockNum);
            updateFileSysInfo(ALOCATE_BLOCK);
            DirEntry newDE[NUM_OF_DIRENT_PER_BLOCK];
            memset(newDE, 0, BLOCK_SIZE);
            strcpy(newDE[0].name, ".");
            newDE[0].inodeNum = newInodeNum;
            strcpy(newDE[1].name, "..");
            newDE[1].inodeNum = parentInodeNum;
            DevWriteBlock(newBlockNum, newDE);
            DevWriteBlock(parentBlockNum, parentDE);

            newInode->dirBlockPtr[0] = newBlockNum;
            PutInode(newInodeNum, newInode);
            free(newInode);
            return newBlockNum;
        }
    }
    return -1;
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
    DevWriteBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
}

int FindParentinode(const char *szFileName, int _inode) //retrun inode
{
    printf("FindParentinode filename : %s\n", szFileName);
    Inode *root = (Inode *) malloc(sizeof(Inode));
    GetInode(_inode, root); // 루트여서 0번째 inode 확인
    char *blkPtr = (char *) malloc(BLOCK_SIZE);

    for (int j = 0; j < NUM_OF_DIRECT_BLOCK_PTR; j++) {
        DevReadBlock(root->dirBlockPtr[j], blkPtr); //이거 일단 0번째 읽게함

        DirEntry *dirPtr = (DirEntry *) blkPtr;
        for (int i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++) { //이건 4번이겠네
            if (strcmp(dirPtr[i].name, szFileName) == 0) {

                //free
                int temp = dirPtr[i].inodeNum;
                free(root);
                free(blkPtr);
                // printf("temp 리턴\n");
                return temp;
            }
        }
    }

    return -1;
}


int OpenFile(const char *szFileName, OpenFlag flag) {

}

int WriteFile(int fileDesc, char *pBuffer, int length) {
//    - open된 파일에 데이터를 저장한다.
//    • Parameters
//    ◦ fileDesc[in]: file descriptor.
//    ◦ pBuffer[in]: 저장할 데이터를 포함하는 메모리의 주소
//    ◦ length[in]: 저장될 데이터의 길이
//    - Return
//    성공하면, 저장된 데이터의 길이 값을 리턴한다. 실패했을때는 -1을 리턴한다.
}

int ReadFile(int fileDesc, char *pBuffer, int length) {
//    - open된 파일에서 데이터를 읽는다.
//    • Parameters
//    ◦ fileDesc[in]: file descriptor.
//    ◦ pBuffer[out]: 읽을 데이터를 저장할 메모리의 주소
//    ◦ length[in]: 읽을 데이터의 길이
//    - Return
//    성공하면, 읽은 데이터의 길이 값을 리턴한다. 실패했을때는 -1을 리턴한다.

}

int CloseFile(int fileDesc) {
    if (pFileDescTable->file[fileDesc].bUsed == 0) {
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
}


int MakeDir(const char *szDirName) {
    Inode *pInode = (Inode *) malloc(sizeof(Inode));
    int *curInodeNum = (int *) malloc(sizeof(int));
    *curInodeNum = 0;
    int *parentBlockNum = (int *) malloc(sizeof(int));

    int inode_index = 0;
    int arr_index = 0;

    int returnDirPtr_index = 0;
    int return_block_index = START_INDEX_DATA_REGION;

    DirEntry *returnDirPtr = (DirEntry *) malloc(BLOCK_SIZE);

    char arr[64][MAX_NAME_LEN + 1]; //폴더 갯수 최대 그냥 64로 했음
    char *input_path = (char *) malloc(sizeof(szDirName));
    strcpy(input_path, szDirName);
    char *temp_Ptr = strtok(input_path, "/");

    while (temp_Ptr != NULL) {
        strcpy(arr[arr_index++], temp_Ptr);
        temp_Ptr = strtok(NULL, "/");
    }
    int oh_array_index = 0;
    int inoindex = 0;
    GetInode(0, pInode);

    for (oh_array_index; oh_array_index < arr_index - 1; oh_array_index++) // 루트에 만들어지면 여기는 그냥 통과가 되버리네
    {
        //*curInodeNum = goDownDir(arr[arr_index-1], pInode, curInodeNum, parentBlockNum);// 이게 돌아주면 될것 같다
    }
//    addNewDir(arr[arr_index-1], pInode, *curInodeNum);

    int i, j;
    int bnum;//이거 왜 1이 들어가는거야 시발
    int newBlockNum = -1;
    DirEntry de[NUM_OF_DIRENT_PER_BLOCK];
    memset(de, 0, BLOCK_SIZE);



    //travel direct pointers
    for (i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {

        bnum = pInode->dirBlockPtr[i];
        if (bnum == 0) { //add new direct pointer and new entry
            int newBlockNum = addNewDirEntry(i, pInode, curInodeNum);
            pInode->dirBlockPtr[i] = newBlockNum;
            pInode->size += BLOCK_SIZE;
            bnum = pInode->dirBlockPtr[i];
            PutInode(curInodeNum, pInode);
        }
        DevReadBlock(bnum, de);

        //add new dir
        for (j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) { //4
            if (strcmp(de[j].name, "") == 0) {
                newBlockNum = addNewDirBlock(i, arr[arr_index - 1], pInode, curInodeNum, bnum, de);
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
        int indirectBlock[BLOCK_SIZE / sizeof(int)];
        memset(indirectBlock, 0, BLOCK_SIZE);
        pInode->indirBlockPtr = indirectBlockNum;
        DevWriteBlock(bnum, indirectBlock);
        PutInode(curInodeNum, pInode);
    }

    int indirectBlock[BLOCK_SIZE / sizeof(int)];
    DevReadBlock(indirectBlockNum, indirectBlock);

    for (i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
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
                newBlockNum = addNewDirBlock(j, arr[arr_index - 1], pInode, curInodeNum, bnum, de);
                DevWriteBlock(indirectBlockNum, indirectBlock);
                return 0;
            }
        }
    }
    return -1;
}

int RemoveDir(const char *szDirName) {

}

int EnumerateDirStatus(const char *szDirName, DirEntryInfo *pDirEntry, int dirEntrys) {


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
