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

int goDownDir(char *target, Inode *curInode, int *curInodeNum, int *bnum) {
    int i, j;
    DirEntry de[NUM_OF_DIRENT_PER_BLOCK];
    //just check direct block
    for (i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {
        *bnum = curInode->dirBlockPtr[i];

        DevReadBlock(*bnum, de);
        for (j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
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
    DevReadBlock(inbnum, indirectBlock);
    for (i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
        *bnum = indirectBlock[i];
        DevReadBlock(*bnum, de);
        for (j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
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

int getParentInode(char *szDirName, char *target, Inode *pInode, int *curInodeNum, int *parentBlockNum) {

    //get root inode
    GetInode(0, pInode);
    int i;
    int nameIndex = 0;

    memset(target, 0, sizeof(target));
    //travel directory
    for (i = 1; i <= strlen(szDirName); i++) {
        if (i >= strlen(szDirName)) { //end of string
            int parentInodeNum = *curInodeNum;
            *curInodeNum = goDownDir(target, pInode, curInodeNum, parentBlockNum);
            return parentInodeNum;
        } else if (szDirName[i] != '/') {
            target[nameIndex++] = szDirName[i];
        } else { // == '/'
            *curInodeNum = goDownDir(target, pInode, curInodeNum, parentBlockNum);
            memset(target, 0, MAX_NAME_LEN);
            nameIndex = 0;
            if (*curInodeNum == -1) {
                //                printf("Invalid Dir Name");
                return -1;
            }
        }
    }
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
    int i, j;

    DirEntry de[NUM_OF_DIRENT_PER_BLOCK];
    memset(de, 0, BLOCK_SIZE);



    //travel direct pointers
    for (i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {


        if (pInode->dirBlockPtr[i] != 0) { //add new ㅡdirect pointer and new entry
            DevReadBlock(pInode->dirBlockPtr[i], (char *) de);
            //add new dir
            for (j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) { //4
                if (strcmp(de[j].name, "") == 0) {
                    addNewDirBlock(i, arr[arr_index - 1], pInode, *curInodeNum,
                                   (int *) pInode->dirBlockPtr[i], de);
                    free(pInode);
                    free(curInodeNum);
                    free(parentBlockNum);
                    return 0;
                }
            }


        } else {
            pInode->dirBlockPtr[i] = addNewDirEntry(i, pInode, *curInodeNum);
            pInode->size += BLOCK_SIZE;
            PutInode(*curInodeNum, pInode);// 여기에 포인터로 접근을 해야하는구나
            DevReadBlock(pInode->dirBlockPtr[i], (char *) de);


            for (j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) { //4
                if (strcmp(de[j].name, "") == 0) {
                    addNewDirBlock(i, arr[arr_index - 1], pInode, *curInodeNum,
                                   (int *) pInode->dirBlockPtr[i], de);
                    free(pInode);
                    free(curInodeNum);
                    free(parentBlockNum);
                    return 0;
                }
            }

        }

    }

    //--------indirect------

    //travel indirect block
    int indirectBlockNum = pInode->indirBlockPtr;

    int indirectBlock[BLOCK_SIZE / sizeof(int)];

    if (indirectBlockNum == 0) { //does't allocate indirect Block yet
        //add new indirect Block

        updateFileSysInfo(ALOCATE_BLOCK);
        indirectBlockNum = GetFreeBlockNum();


        memset(indirectBlock, 0, BLOCK_SIZE);
        pInode->indirBlockPtr = indirectBlockNum;
        DevWriteBlock(GetFreeBlockNum(), (char *) indirectBlock);
        SetBlockBitmap(GetFreeBlockNum());
        PutInode(*curInodeNum, pInode);

    }

    DevReadBlock(indirectBlockNum, (char *) indirectBlock);

    for (i = 0; i < BLOCK_SIZE / sizeof(int); i++) {


        if (indirectBlock[i] == 0) { //add new block for indirectBlock

            updateFileSysInfo(ALOCATE_BLOCK);
            pInode->size += BLOCK_SIZE;
            indirectBlock[i] = GetFreeBlockNum();;
            DirEntry *newDirEntry = (DirEntry *) malloc(BLOCK_SIZE);
            memset(newDirEntry, 0, BLOCK_SIZE);
            DevWriteBlock(GetFreeBlockNum(), (char *) newDirEntry);
            SetBlockBitmap(GetFreeBlockNum());
            DevWriteBlock(indirectBlockNum, (char *) indirectBlock);
            free(newDirEntry);
        }
        DirEntry temp_de[NUM_OF_DIRENT_PER_BLOCK];
        DevReadBlock(indirectBlock[i], temp_de);
        for (j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (strcmp(temp_de[j].name, "") == 0) { //get empty dirent number
                addNewDirBlock(j, arr[arr_index - 1], pInode, *curInodeNum, (int *) indirectBlock[i], temp_de);
                DevWriteBlock(indirectBlockNum, (char *) indirectBlock);
                free(pInode);//내가 알아서 추가해봄
                free(curInodeNum);
                free(parentBlockNum);


                return 0;
            }
        }
    }
    return -1;
}

int RemoveDir(const char *szDirName) {

}

int EnumerateBlocks(DirEntry *de, DirEntryInfo *pDirEntry, int dirEntrys, int count) {
    int i;
    Inode *tempInode = (Inode *) malloc(sizeof(Inode));

    for (i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++) {
        if (count >= dirEntrys)
            break;
        if (de[i].inodeNum != 0) {   //copy Directory Information
            //            if(strcmp(de[i].name,".")==0||strcmp(de[i].name,"..")==0)
            //                continue;
            //            if(de[i].inodeNum==0 || strcmp(de[i].name,"")==0)
            //                continue;
            if (strcmp(de[i].name, "") == 0)
                continue;
            pDirEntry[count].inodeNum = de[i].inodeNum;
            strcpy(pDirEntry[count].name, de[i].name);
            GetInode(de[i].inodeNum, tempInode);
            pDirEntry[count].type = tempInode->type;
            count++;
        }
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

    for (oh_array_index; oh_array_index < arr_index; oh_array_index++) // 루트에 만들어지면 여기는 그냥 통과가 되버리네
    {
        *curInodeNum = goDownDir(arr[oh_array_index], pInode, curInodeNum, parentBlockNum);// 이게 돌아주면 될것 같다


    }
    if (*curInodeNum == -1) {
        free(pInode);
        free(curInodeNum);
        free(parentBlockNum);
        return -1;
    }
    //여기는 다 똑같음
    Inode *parentInode = (Inode *) malloc(sizeof(Inode));
    GetInode(*curInodeNum, parentInode);
    int i, count = 0;
    int bnum;

    //direct
    DirEntry de[NUM_OF_DIRENT_PER_BLOCK];
    for (i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {
        bnum = pInode->dirBlockPtr[i];
        if (bnum == 0)
            continue;
        DevReadBlock(bnum, de);
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
        DevReadBlock(inbnum, indirectBlock);
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
