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


int Parent_find(const char *currentFileName, int inode_index, DirEntry *returnDirPtr, int *returnDirPtr_index,
                int *return_block_index);

int Parent_find(const char *currentFileName, int inode_index, DirEntry *returnDirPtr, int *returnDirPtr_index,
                int *return_block_index) {
    Inode *current_root = (Inode *) malloc(sizeof(Inode));
    GetInode(inode_index, current_root);

    char *block_buf = (char *) malloc(BLOCK_SIZE);
    DirEntry *dir_block = (DirEntry *) block_buf;

    for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {//다이렉트 노트 확인
        DevReadBlock(current_root->dirBlockPtr[i], block_buf);
        if (current_root->dirBlockPtr[i] == 0) {
            break;
        }
        for (int index_direct = 0; index_direct < NUM_OF_DIRENT_PER_BLOCK; index_direct++) {
            if (strcmp(dir_block[index_direct].name, currentFileName) == 0) {
                for (int dir_index = 0; dir_index < NUM_OF_DIRENT_PER_BLOCK; dir_index++) {
                    returnDirPtr[dir_index].inodeNum = dir_block[dir_index].inodeNum;
                    strcpy(returnDirPtr[dir_index].name, dir_block[dir_index].name);
                }
                *returnDirPtr_index = index_direct;
                *return_block_index = current_root->dirBlockPtr[i];

                return returnDirPtr[index_direct].inodeNum;

            }

        }
    }
    if (current_root->indirBlockPtr != 0) {


        for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
            DevReadBlock(current_root->indirBlockPtr, block_buf);
            int *indirect = (int *) block_buf;
            char *block_indirect_buf = (char *) malloc(BLOCK_SIZE);

            if (indirect[i] == 0) {

                break;

            } else {
                DevReadBlock(indirect[i], block_indirect_buf);

            }


            for (int blockindex = 0; blockindex < NUM_OF_DIRENT_PER_BLOCK; blockindex++) {
                DirEntry *dir_temp_block = (DirEntry *) block_indirect_buf;

                if (strcmp(dir_temp_block[blockindex].name, currentFileName) == 0) {
                    for (int dir_index = 0; dir_index < NUM_OF_DIRENT_PER_BLOCK; dir_index++) {
                        returnDirPtr[dir_index].inodeNum = dir_temp_block[dir_index].inodeNum;
                        strcpy(returnDirPtr[dir_index].name, dir_temp_block[dir_index].name);
                    }
                    *returnDirPtr_index = blockindex;
                    *return_block_index = indirect[i];

                    return returnDirPtr[blockindex].inodeNum;
                }

            }
        }
    }

    return 0;
}

int current_find(int inode_index, DirEntry *returnDirPtr, int *returnDirPtr_index, int *return_block_index) {

    Inode *current_root = (Inode *) malloc(sizeof(Inode));
    GetInode(inode_index, current_root);

    char *block_buf = (char *) malloc(BLOCK_SIZE);
    DirEntry *dir_block = (DirEntry *) block_buf;

    for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {//다이렉트 노트 확인

        DevReadBlock(current_root->dirBlockPtr[i], block_buf);
        if (current_root->dirBlockPtr[i] == 0) {
            break;
        }
        for (int index_direct = 0; index_direct < NUM_OF_DIRENT_PER_BLOCK; index_direct++) {

            if (strcmp(dir_block[index_direct].name, "") == 0) {
                for (int dir_index = 0; dir_index < NUM_OF_DIRENT_PER_BLOCK; dir_index++) {
                    returnDirPtr[dir_index].inodeNum = dir_block[dir_index].inodeNum;
                    strcpy(returnDirPtr[dir_index].name, dir_block[dir_index].name);
                }
                *returnDirPtr_index = index_direct;
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
            DirEntry *dir_temp_block = (DirEntry *) block_indirect_buf;

            if (indirect[i] == 0)
                break;
            for (int index_direct = 0; index_direct < NUM_OF_DIRENT_PER_BLOCK; index_direct++) {
                if (strcmp(dir_temp_block[index_direct].name, "") != 0) {

                } else {
                    for (int dir_index = 0; dir_index < NUM_OF_DIRENT_PER_BLOCK; dir_index++) {
                        returnDirPtr[dir_index].inodeNum = dir_temp_block[dir_index].inodeNum;
                        strcpy(returnDirPtr[dir_index].name, dir_temp_block[dir_index].name);
                    }
                    *returnDirPtr_index = index_direct;
                    *return_block_index = indirect[i];

                    return 1;
                }
            }
        }
    }


    return 0;
}

int OpenFile(const char *szFileName, OpenFlag flag) {
    int oh_array_index = 0;
    int arr_index = 0;
    int inode_index = 0;
    DirEntry *returnDirPtr = (DirEntry *) malloc(BLOCK_SIZE);
    int returnDirPtr_index = 0;
    int return_block_index = START_INDEX_DATA_REGION;
    char arr[64][MAX_NAME_LEN + 1]; //폴더 갯수 최대 그냥 64로 했음
    char *input_path = (char *) malloc(sizeof(szFileName));
    strcpy(input_path, szFileName);
    char *temp_Ptr = strtok(input_path, "/");

    while (temp_Ptr != NULL) {
        strcpy(arr[arr_index++], temp_Ptr);
        temp_Ptr = strtok(NULL, "/");
    }
    for (oh_array_index; oh_array_index < arr_index; oh_array_index++) // 루트에 만들어지면 여기는 그냥 통과가 되버리네
    {
        int temp = Parent_find(arr[oh_array_index], inode_index, returnDirPtr, &returnDirPtr_index,
                               &return_block_index);
        if (temp == 0)
            break;


        inode_index = temp;
        if (arr_index == oh_array_index + 1) {
            FileDesc *fdPtr = (FileDesc *) pFileDescTable;
            for (int i = 0; i < MAX_FD_ENTRY_LEN; i++) {

                if (pFileDescTable->file[i].bUsed == 0) {
                    pFileDescTable->file[i].inodeNum = returnDirPtr[returnDirPtr_index].inodeNum;
                    pFileDescTable->file[i].bUsed = 1;
                    pFileDescTable->file[i].fileOffset = 0;
                    return i;
                }
            }
        }
    }
    if (flag != OPEN_FLAG_CREATE || oh_array_index >= arr_index)
        return -1;

    int temp = current_find(inode_index, returnDirPtr, &returnDirPtr_index, &return_block_index);//이거 용도 뭐지
    DirEntry *pDirEntry = returnDirPtr;
    int parent_dri_buf_index = returnDirPtr_index;
    int parent_inode_index = inode_index;
    int parent_block_index = return_block_index;


    int fFdIndex = 0;
    for (int i = 0; i < MAX_FD_ENTRY_LEN; i++) {
        if (pFileDescTable->file[i].bUsed == 0) {
            fFdIndex = i;
            pFileDescTable->file[i].inodeNum = GetFreeInodeNum();
            pFileDescTable->file[i].bUsed = 1;
            pFileDescTable->file[i].fileOffset = 0;

            break;
        }
    }
    DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
    pFileSysInfo->numAllocBlocks++;
    pFileSysInfo->numFreeBlocks--;
    DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
    if (temp == 0) //찼을때
    {
        //get parent tempinodebuf
        Inode *parent_inode_buf = (Inode *) malloc(sizeof(Inode));
        GetInode(parent_inode_index, parent_inode_buf);

        if (parent_inode_buf->dirBlockPtr[1] != 0) {

            if (parent_inode_buf->indirBlockPtr != 0) {

                char *buf = (char *) malloc(BLOCK_SIZE);
                DevReadBlock(parent_inode_buf->indirBlockPtr, buf);

                for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
                    if (buf[i] == 0) {
                        DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                        pFileSysInfo->numAllocBlocks++;
                        pFileSysInfo->numFreeBlocks--;
                        DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                        buf[i] = (char) GetFreeBlockNum();
                        DevWriteBlock(parent_inode_buf->indirBlockPtr, buf);


                        parent_block_index = GetFreeBlockNum();
                        SetBlockBitmap(GetFreeBlockNum());

                    }
                }


            } else {
                DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                pFileSysInfo->numAllocBlocks++;
                pFileSysInfo->numFreeBlocks--;
                DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                parent_inode_buf->indirBlockPtr = GetFreeBlockNum();
                PutInode(parent_inode_index, parent_inode_buf);
                SetBlockBitmap(GetFreeBlockNum());

                parent_block_index = GetFreeBlockNum();

                SetBlockBitmap(GetFreeBlockNum());

                char *pIndirBlkPtr = (char *) malloc(BLOCK_SIZE);
                int *pIndirPtr = (int *) pIndirBlkPtr;
                pIndirPtr[0] = GetFreeBlockNum();
                DevWriteBlock(GetFreeBlockNum(), (char *) pIndirPtr);
            }


        } else {

            parent_block_index = GetFreeBlockNum();
            parent_inode_buf->dirBlockPtr[1] = GetFreeBlockNum();
            PutInode(parent_inode_index, parent_inode_buf);

            SetBlockBitmap(GetFreeBlockNum());

        }
    }


    SetInodeBitmap(GetFreeInodeNum());
    Inode *tempinodebuf = (Inode *) malloc(sizeof(Inode));
    DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
    pFileSysInfo->numAllocInodes++;
    DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);

    strcpy(pDirEntry[parent_dri_buf_index].name, arr[arr_index - 1]);
    pDirEntry[parent_dri_buf_index].inodeNum = GetFreeInodeNum();
    DevWriteBlock(parent_block_index, (char *) pDirEntry);
    GetInode(GetFreeInodeNum(), tempinodebuf);
    tempinodebuf->dirBlockPtr[0] = 0;
    tempinodebuf->dirBlockPtr[1] = 0;
    tempinodebuf->indirBlockPtr = 0;

    tempinodebuf->size = 0;
    tempinodebuf->type = FILE_TYPE_FILE;

    PutInode(GetFreeInodeNum(), tempinodebuf);
    return fFdIndex;


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
    for (oh_array_index; oh_array_index < arr_index; oh_array_index++) // 루트에 만들어지면 여기는 그냥 통과가 되버리네
    {
        int temp = Parent_find(arr[oh_array_index], inode_index, returnDirPtr, &returnDirPtr_index,
                               &return_block_index);
        if (temp == 0)
            break;


        inode_index = temp;
    }

    int temp = current_find(inode_index, returnDirPtr, &returnDirPtr_index, &return_block_index);//이거 용도 뭐지
    DirEntry *pDirEntry = returnDirPtr;
    int parent_dri_buf_index = returnDirPtr_index;
    int parent_inode_index = inode_index;
    int parent_block_index = return_block_index;
    Inode *parent_inode_buf = (Inode *) malloc(sizeof(Inode));
    GetInode(parent_inode_index, parent_inode_buf);
    if (temp == 0) {
        parent_dri_buf_index = 0;

        memset(pDirEntry, 0, BLOCK_SIZE);

        if (parent_inode_buf->dirBlockPtr[1] == 0) {
            DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
            pFileSysInfo->numAllocBlocks++;
            pFileSysInfo->numFreeBlocks--;
            DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
            parent_inode_buf->dirBlockPtr[1] = GetFreeBlockNum();
            PutInode(parent_inode_index, parent_inode_buf);
            parent_block_index = GetFreeBlockNum();
            SetBlockBitmap(GetFreeBlockNum());

        } else {
            if (parent_inode_buf->indirBlockPtr == 0) {
                DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                pFileSysInfo->numAllocBlocks++;
                pFileSysInfo->numFreeBlocks--;
                DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                int parent_free_block_index = GetFreeBlockNum();
                parent_inode_buf->indirBlockPtr = parent_free_block_index;
                PutInode(parent_inode_index, parent_inode_buf);
                SetBlockBitmap(parent_free_block_index);
                char *pIndirBlkPtr = (char *) malloc(BLOCK_SIZE);
                int *pIndirPtr = (int *) pIndirBlkPtr;
                pIndirPtr[0] = GetFreeBlockNum();
                DevWriteBlock(parent_free_block_index, (char *) pIndirPtr);

                parent_block_index = GetFreeBlockNum();
                SetBlockBitmap(GetFreeBlockNum());

            } else {
                int *buf = (int *) malloc(BLOCK_SIZE);
                DevReadBlock(parent_inode_buf->indirBlockPtr, (char *) buf);

                for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
                    if (buf[i] == 0) {
                        DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                        pFileSysInfo->numAllocBlocks++;
                        pFileSysInfo->numFreeBlocks--;
                        DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
                        buf[i] = GetFreeBlockNum();
                        DevWriteBlock(parent_inode_buf->indirBlockPtr, (char *) buf);
                        parent_block_index = GetFreeBlockNum();
                        SetBlockBitmap(GetFreeBlockNum());

                    }
                }
            }
        }

    }
    strncpy(pDirEntry[parent_dri_buf_index].name, arr[oh_array_index],
            sizeof(pDirEntry[parent_dri_buf_index].name) - 1); //strcpy는 안좋다니까 strncpy로 함
    pDirEntry[parent_dri_buf_index].inodeNum = GetFreeInodeNum();
    DevWriteBlock(parent_block_index, (char *) pDirEntry);
    char *current_block_buf = (char *) malloc(BLOCK_SIZE);
    DirEntry *cDirEntry = (DirEntry *) current_block_buf;
    strncpy(cDirEntry[0].name, ".", 1);
    cDirEntry[0].inodeNum = GetFreeInodeNum();
    strncpy(cDirEntry[1].name, "..", 2);
    cDirEntry[1].inodeNum = parent_inode_index;
    DevWriteBlock(GetFreeBlockNum(), (char *) cDirEntry);
    Inode *current_inode_buf = (Inode *) malloc(sizeof(Inode));
    DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
    pFileSysInfo->numAllocBlocks++;
    pFileSysInfo->numFreeBlocks--;
    pFileSysInfo->numAllocInodes++;
    DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
    GetInode(GetFreeInodeNum(), current_inode_buf);
    current_inode_buf->indirBlockPtr = 0;
    current_inode_buf->dirBlockPtr[0] = GetFreeBlockNum();
    current_inode_buf->dirBlockPtr[1] = 0;
    current_inode_buf->type = FILE_TYPE_DIR;
    current_inode_buf->size = 0;
    PutInode(GetFreeInodeNum(), current_inode_buf);
    SetInodeBitmap(GetFreeInodeNum());
    SetBlockBitmap(GetFreeBlockNum());


    return 0;
}

int RemoveDir(const char *szDirName) {
    int arr_index = 0;

    int inode_index = 0;
    int oh_array_index = 0;

    DirEntry *returnDirPtr = (DirEntry *) malloc(BLOCK_SIZE);
    int returnDirPtr_index = 0;
    int return_block_index = START_INDEX_DATA_REGION;
    char arr[64][MAX_NAME_LEN + 1]; //폴더 갯수 최대 그냥 64로 했음
    char *input_path = (char *) malloc(sizeof(szDirName));
    strcpy(input_path, szDirName);
    char *temp_Ptr = strtok(input_path, "/");
    while (temp_Ptr != NULL) {
        strcpy(arr[arr_index++], temp_Ptr);
        temp_Ptr = strtok(NULL, "/");
    }
    for (oh_array_index; oh_array_index < arr_index; oh_array_index++) // 루트에 만들어지면 여기는 그냥 통과가 되버리네
    {
        int temp = Parent_find(arr[oh_array_index], inode_index, returnDirPtr, &returnDirPtr_index,
                               &return_block_index);
        if (temp == 0)
            break;


        inode_index = temp;
    }

    DirEntry *pDirEntry = returnDirPtr;
    Inode *current_inode_buf = (Inode *) malloc(sizeof(Inode));
    int current_inode_index = pDirEntry[returnDirPtr_index].inodeNum;
    GetInode(current_inode_index, current_inode_buf);
    int parent_dri_buf_index = returnDirPtr_index;
    int parent_block_index = return_block_index;//이거 바꿔야겠다.



    char *buf = (char *) malloc(BLOCK_SIZE);
    for (int index = 0; index < NUM_OF_DIRECT_BLOCK_PTR; index++) {
        if (current_inode_buf->dirBlockPtr[index] == 0) {


        } else {
            for (int index_direct = 0; index_direct < NUM_OF_DIRENT_PER_BLOCK; index_direct++) {
                DevReadBlock(current_inode_buf->dirBlockPtr[index], buf);
                DirEntry *dir_block = (DirEntry *) buf;

                if ((strcmp(dir_block[index_direct].name, ".") != 0 &&
                     strcmp(dir_block[index_direct].name, "..") != 0) &&
                    strcmp(dir_block[index_direct].name, "") != 0) {
                    return 0;
                }
            }
        }


    }
    DevWriteBlock(current_inode_buf->dirBlockPtr[0], buf);

    char *inderect_buf = (char *) malloc(BLOCK_SIZE);
    if (current_inode_buf->indirBlockPtr != 0) {
        DevReadBlock(current_inode_buf->indirBlockPtr, buf);
        for (int index = 0; index < BLOCK_SIZE / sizeof(int); index++) {
            if (buf[index] != 0) {
                for (int l = 0; l < NUM_OF_DIRENT_PER_BLOCK; l++) {
                    DevReadBlock(buf[index], inderect_buf);
                    DirEntry *dir_block = (DirEntry *) inderect_buf;
                    if (strcmp(dir_block[l].name, "") != 0)
                        break;
                    if (l == NUM_OF_DIRENT_PER_BLOCK - 1)
                        return 0;
                }
            }

        }
        DevWriteBlock(current_inode_buf->dirBlockPtr[0], buf);

    }


    ResetBlockBitmap(current_inode_buf->dirBlockPtr[0]);
    ResetInodeBitmap(pDirEntry[parent_dri_buf_index].inodeNum);


    memset(current_inode_buf, 0, sizeof(Inode));
    PutInode(pDirEntry[parent_dri_buf_index].inodeNum, current_inode_buf);


    strcpy(pDirEntry[parent_dri_buf_index].name, "");
    pDirEntry[parent_dri_buf_index].inodeNum = 0;
    DevWriteBlock(parent_block_index, (char *) pDirEntry);

    DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
    pFileSysInfo->numAllocBlocks--;
    pFileSysInfo->numFreeBlocks++;
    DevWriteBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
    return -1;
}

int EnumerateDirStatus(const char *szDirName, DirEntryInfo *pDirEntry, int dirEntrys) {
    int arr_index = 0;

    int inode_index = 0;
    int count = 0;

    char arr[64][MAX_NAME_LEN + 1]; //폴더 갯수 최대 그냥 64로 했음
    char *input_path = (char *) malloc(sizeof(szDirName));
    strcpy(input_path, szDirName);
    char *temp_Ptr = strtok(input_path, "/");

    while (temp_Ptr != NULL) {
        strcpy(arr[arr_index++], temp_Ptr);
        temp_Ptr = strtok(NULL, "/");
    }
    int oh_array_index = 0;
    DirEntry *returnDirPtr = (DirEntry *) malloc(BLOCK_SIZE);
    int returnDirPtr_index = 0;
    int return_block_index = START_INDEX_DATA_REGION;
    //find
    for (oh_array_index; oh_array_index < arr_index; oh_array_index++) // 루트에 만들어지면 여기는 그냥 통과가 되버리네
    {
        int temp = Parent_find(arr[oh_array_index], inode_index, returnDirPtr, &returnDirPtr_index,
                               &return_block_index);
        if (temp == 0)
            break;


        inode_index = temp;
    }
    Inode *inode_buf = (Inode *) malloc(sizeof(Inode));

    GetInode(returnDirPtr[returnDirPtr_index].inodeNum, inode_buf);
    Inode *current_inode_buf = (Inode *) malloc(sizeof(Inode));


    for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {
        if (dirEntrys < count)
            return count;
        if (inode_buf->dirBlockPtr[i] != 0) {

            for (int index_direct = 0; index_direct < NUM_OF_DIRENT_PER_BLOCK; index_direct++) {
                char *buf = (char *) malloc(BLOCK_SIZE);
                DevReadBlock(inode_buf->dirBlockPtr[i], buf);
                DirEntry *dir_block = (DirEntry *) buf;
                if (strcmp(dir_block[index_direct].name, "") != 0) {
                    GetInode(dir_block[index_direct].inodeNum, current_inode_buf);
                    strcpy(pDirEntry[count].name, dir_block[index_direct].name);
                    pDirEntry[count].type = (FileType) current_inode_buf->type;
                    pDirEntry[count].inodeNum = dir_block[index_direct].inodeNum;
                    count++;
                }
            }
        }
    }
    if (inode_buf->indirBlockPtr != 0) {

        for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
            char *buf = (char *) malloc(BLOCK_SIZE);
            DevReadBlock(inode_buf->indirBlockPtr, buf);
            int *indirect_buf = (int *) buf;
            if (dirEntrys < count)
                return count;
            if (indirect_buf[i] != 0) {

                for (int index_direct = 0; index_direct < NUM_OF_DIRENT_PER_BLOCK; index_direct++) {
                    char *buf2 = (char *) malloc(BLOCK_SIZE);
                    DevReadBlock(indirect_buf[i], buf2);
                    DirEntry *dir_block = (DirEntry *) buf2;
                    if (strcmp(dir_block[index_direct].name, "") == 0) {


                    } else {
                        GetInode(dir_block[index_direct].inodeNum, current_inode_buf);

                        pDirEntry[count].inodeNum = dir_block[index_direct].inodeNum;

                        pDirEntry[count].type = (FileType) current_inode_buf->type;

                        strcpy(pDirEntry[count].name, dir_block[index_direct].name);

                        count++;
                    }
                }
            }
        }
    }

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
