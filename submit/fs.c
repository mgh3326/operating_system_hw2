//-fs.c: 과제1에서 구현한 코드를 이 파일에 넣고, 빈 함수를 구현하면 됩니다.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "disk.h"
#include "fs.h"

FileDescTable *pFileDescTable = NULL;

int findInode(char *find_filename, Inode *curInode)
{ //이거는 많이 쓸거 같다

    DirEntry directory_entry[NUM_OF_DIRENT_PER_BLOCK];
    for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++)
    {
        DevReadBlock(curInode->dirBlockPtr[i], (char *)directory_entry);
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++)
        {
            if (strcmp(directory_entry[j].name, find_filename) == 0)
            {
                GetInode(directory_entry[j].inodeNum, curInode);
                return directory_entry[j].inodeNum;
            }
        }
    }

    int indirect_block[BLOCK_SIZE / sizeof(int)];
    DevReadBlock(curInode->indirBlockPtr, (char *)indirect_block);
    for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++)
    {
        DevReadBlock(indirect_block[i], (char *)directory_entry);
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++)
        {
            if (strcmp(directory_entry[j].name, find_filename) == 0)
            {
                GetInode(directory_entry[j].inodeNum, curInode);
                return directory_entry[j].inodeNum;
            }
        }
    }
    return -1;
}

int OpenFile(const char *szFileName, OpenFlag flag)
{
    int curInodeNum = 0;
    int arr_index = 0;
    char arr[64][MAX_NAME_LEN + 1]; //폴더 갯수 최대 그냥 64로 했음
    char *input_path = (char *)malloc(sizeof(szFileName));
    strcpy(input_path, szFileName);
    char *temp_array = strtok(input_path, "/");
    while (temp_array != NULL)
    {
        strcpy(arr[arr_index++], temp_array);
        temp_array = strtok(NULL, "/");
    }
    int oh_array_index = 0;
    Inode *pInode = (Inode *)malloc(sizeof(Inode));
    GetInode(0, pInode);
    int parentInodeNum = 0;
    for (oh_array_index; oh_array_index < arr_index - 1; oh_array_index++) // 루트에 만들어지면 여기는 그냥 통과가 되버리네
    {
        parentInodeNum = findInode(arr[oh_array_index], pInode); // 이게 돌아주면 될것 같다
    }

    if (flag == OPEN_FLAG_READWRITE)
    {
        curInodeNum = findInode(arr[oh_array_index], pInode); // 이게 돌아주면 될것 같다
        for (int i = 0; i < MAX_FD_ENTRY_LEN; i++)
        {
            if (pFileDescTable->file[i].bUsed == 0)
            {
                pFileDescTable->file[i].bUsed = 1;
                pFileDescTable->file[i].inodeNum = curInodeNum;
                pFileDescTable->file[i].fileOffset = 0;
                return i;
            }
        }
    }
    else if (flag == OPEN_FLAG_CREATE)
    {

        int new_block_num;
        Inode *parentInode = (Inode *)malloc(BLOCK_SIZE);
        GetInode(parentInodeNum, parentInode);
        DirEntry directory_entry[NUM_OF_DIRENT_PER_BLOCK];

        for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++)
        {

            int block_num = parentInode->dirBlockPtr[i];
            if (parentInode->dirBlockPtr[i] == 0)
            {

                DevReadBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
                pFileSysInfo->numAllocBlocks++;
                pFileSysInfo->numFreeBlocks--;
                DevWriteBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);

                DirEntry newDirEntry[NUM_OF_DIRENT_PER_BLOCK];
                memset(newDirEntry, 0, BLOCK_SIZE);

                parentInode->dirBlockPtr[i] = GetFreeBlockNum();

                DevWriteBlock(GetFreeBlockNum(), (char *)newDirEntry);

                parentInode->dirBlockPtr[i] = GetFreeBlockNum();
                SetBlockBitmap(GetFreeBlockNum());
                block_num = parentInode->dirBlockPtr[i];
                parentInode->size = (short)(BLOCK_SIZE + parentInode->size);

                PutInode(parentInodeNum, parentInode);
            }
            DevReadBlock(parentInode->dirBlockPtr[i], (char *)directory_entry);

            for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++)
            {
                if (strcmp(directory_entry[j].name, "") == 0)
                {

                    for (int i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++)
                    {
                        if (strcmp(directory_entry[i].name, "") == 0)
                        {

                            int new_inode_num = GetFreeInodeNum();

                            DevReadBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
                            pFileSysInfo->numAllocInodes++;
                            DevWriteBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
                            SetInodeBitmap(new_inode_num);
                            Inode *new_inode = (Inode *)malloc(BLOCK_SIZE);
                            memset(new_inode, 0, sizeof(new_inode));
                            new_inode->type = FILE_TYPE_FILE;
                            new_inode->size = 0;

                            PutInode(new_inode_num, new_inode);

                            strcpy(directory_entry[i].name, arr[arr_index - 1]);
                            directory_entry[i].inodeNum = new_inode_num;
                            DevWriteBlock(block_num, (char *)directory_entry);

                            free(new_inode);
                            free(parentInode);

                            for (int i = 0; i < MAX_FD_ENTRY_LEN; i++)
                            {
                                if (pFileDescTable->file[i].bUsed == 0)
                                {
                                    pFileDescTable->file[i].bUsed = 1;
                                    pFileDescTable->file[i].inodeNum = new_inode_num;
                                    pFileDescTable->file[i].fileOffset = 0;
                                    free(pInode);
                                    return i;
                                }
                            }
                        }
                    }
                }
            }
        }
        int indirectBlockNum = parentInode->indirBlockPtr;
        if (parentInode->indirBlockPtr == 0)
        {
            new_block_num = GetFreeBlockNum();

            DevReadBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
            pFileSysInfo->numAllocBlocks++;
            pFileSysInfo->numFreeBlocks--;
            DevWriteBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
            SetBlockBitmap(new_block_num);
            indirectBlockNum = new_block_num;

            int indirect_block[BLOCK_SIZE / sizeof(int)];
            memset(indirect_block, 0, BLOCK_SIZE);
            parentInode->indirBlockPtr = indirectBlockNum;
            DevWriteBlock(new_block_num, (char *)indirect_block);
            PutInode(parentInodeNum, parentInode);
        }

        int indirect_block[BLOCK_SIZE / sizeof(int)];
        DevReadBlock(indirectBlockNum, (char *)indirect_block);

        for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++)
        {

            if (indirect_block[i] == 0)
            {
                new_block_num = GetFreeBlockNum();
                SetBlockBitmap(new_block_num);
                DevReadBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
                pFileSysInfo->numAllocBlocks++;
                pFileSysInfo->numFreeBlocks--;
                DevWriteBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
                parentInode->size = (short)(BLOCK_SIZE + parentInode->size);
                indirect_block[i] = new_block_num;
                DirEntry *newDirEntry = (DirEntry *)malloc(BLOCK_SIZE);
                memset(newDirEntry, 0, BLOCK_SIZE);
                DevWriteBlock(new_block_num, (char *)newDirEntry);
                DevWriteBlock(indirectBlockNum, (char *)indirect_block);
                free(newDirEntry);
            };
            DevReadBlock(indirect_block[i], (char *)directory_entry);

            for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++)
            {
                if (strcmp(directory_entry[j].name, "") == 0)
                {

                    for (int k = 0; k < NUM_OF_DIRENT_PER_BLOCK; k++)
                    {
                        if (strcmp(directory_entry[k].name, "") == 0)
                        {

                            int new_inode_num = GetFreeInodeNum();
                            DevReadBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
                            pFileSysInfo->numAllocInodes++;
                            DevWriteBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
                            SetInodeBitmap(new_inode_num);
                            Inode *new_inode = (Inode *)malloc(BLOCK_SIZE);
                            memset(new_inode, 0, sizeof(new_inode));
                            new_inode->type = FILE_TYPE_FILE;
                            new_inode->size = 0;

                            PutInode(new_inode_num, new_inode);

                            strcpy(directory_entry[k].name, arr[arr_index - 1]);
                            directory_entry[k].inodeNum = new_inode_num;
                            DevWriteBlock(indirect_block[k], (char *)directory_entry);

                            free(new_inode);
                            DevWriteBlock(indirectBlockNum, (char *)indirect_block);
                            free(parentInode);
                            for (int l = 0; l < MAX_FD_ENTRY_LEN; l++)
                            {
                                if (pFileDescTable->file[l].bUsed == 0)
                                {
                                    pFileDescTable->file[l].bUsed = 1;
                                    pFileDescTable->file[l].inodeNum = new_inode_num;
                                    pFileDescTable->file[l].fileOffset = 0;
                                    free(pInode);

                                    return l;
                                }
                            }
                        }
                    }
                }
            }
        }
        free(parentInode);
        return -1;
    }
    //파일이 있다고 가정하에 하는거여서 문제가 발생하는듯 하다.
}

void BlockAdd(Inode *file_inode, int inodeNum, int writeBlockNum)
{
    for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++)
    {
        if (file_inode->dirBlockPtr[i] == 0)
        {
            file_inode->dirBlockPtr[i] = writeBlockNum;
            PutInode(inodeNum, file_inode);
            return;
        }
    }
    if (file_inode->indirBlockPtr == 0)
    {
        DevReadBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
        pFileSysInfo->numAllocBlocks++;
        pFileSysInfo->numFreeBlocks--;
        DevWriteBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
        file_inode->indirBlockPtr = GetFreeBlockNum();
        SetBlockBitmap(GetFreeBlockNum());
        int indirect_block[BLOCK_SIZE / sizeof(int)];
        memset(indirect_block, 0, BLOCK_SIZE);

        DevWriteBlock(file_inode->indirBlockPtr, (char *)indirect_block);
        PutInode(inodeNum, file_inode);
    }
    int indirect_block[BLOCK_SIZE / sizeof(int)];
    DevReadBlock(file_inode->indirBlockPtr, (char *)indirect_block);
    for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++)
    {
        if (indirect_block[i] == 0)
        {
            char *newFileBlock = (char *)malloc(BLOCK_SIZE);
            indirect_block[i] = writeBlockNum;
            memset(newFileBlock, 0, BLOCK_SIZE);
            DevWriteBlock(writeBlockNum, newFileBlock);
            DevWriteBlock(file_inode->indirBlockPtr, (char *)indirect_block);
            free(newFileBlock);
            return;
        }
    }
}

int WriteFile(int fileDesc, char *pBuffer, int length)
{

    Inode *file_inode = (Inode *)malloc(sizeof(Inode));

    GetInode(pFileDescTable->file[fileDesc].inodeNum, file_inode);

    char *writeBlock = (char *)malloc(BLOCK_SIZE);
    memset(writeBlock, 0, sizeof(BLOCK_SIZE));
    int writeBlockNum = 0;
    int oh_index;
    int totalWriteSize = 0;
    int saveFileBlockNum;

    while (totalWriteSize < length)
    {
        oh_index = pFileDescTable->file[fileDesc].fileOffset + totalWriteSize;

        if (file_inode->size % BLOCK_SIZE == 0)
        {
            writeBlockNum = GetFreeBlockNum();
            DevWriteBlock(writeBlockNum, writeBlock);
            DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
            pFileSysInfo->numAllocBlocks++;
            pFileSysInfo->numFreeBlocks--;
            DevWriteBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
            SetBlockBitmap(writeBlockNum);
            BlockAdd(file_inode, pFileDescTable->file[fileDesc].inodeNum, writeBlockNum);
        }
        if (oh_index / BLOCK_SIZE == 0)
            saveFileBlockNum = file_inode->dirBlockPtr[0];
        else if (oh_index / BLOCK_SIZE == 1)
            saveFileBlockNum = file_inode->dirBlockPtr[1];
        else
        {
            int indirect_block[BLOCK_SIZE / sizeof(int)];
            DevReadBlock(file_inode->indirBlockPtr, (char *)indirect_block);
            saveFileBlockNum = indirect_block[(oh_index / BLOCK_SIZE) - 2];
        }
        char saveBlock[BLOCK_SIZE];
        DevReadBlock(saveFileBlockNum, saveBlock);

        int writeStartIndex = BLOCK_SIZE - (((oh_index / BLOCK_SIZE + 1) * BLOCK_SIZE) - oh_index);
        for (int i = 0; i < (((oh_index / BLOCK_SIZE + 1) * BLOCK_SIZE) - oh_index); i++)
        {
            saveBlock[writeStartIndex] = pBuffer[totalWriteSize];
            writeStartIndex++;
            totalWriteSize++;
            if (totalWriteSize >= length || totalWriteSize >= (int)strlen(pBuffer))
            {
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

int ReadFile(int fileDesc, char *pBuffer, int length)
{
    Inode *file_inode;
    file_inode = (Inode *)malloc(sizeof(Inode));
    GetInode(pFileDescTable->file[fileDesc].inodeNum, file_inode);
    char *readBlock = (char *)malloc(BLOCK_SIZE);
    int readBlockNum;
    int oh_index;
    int totalReadSize = 0;
    while (totalReadSize < length)
    {
        oh_index = pFileDescTable->file[fileDesc].fileOffset + totalReadSize;
        if (oh_index >= file_inode->size)
            break;

        if (oh_index / BLOCK_SIZE == 0)
            readBlockNum = file_inode->dirBlockPtr[0];
        else if (oh_index / BLOCK_SIZE == 1)
            readBlockNum = file_inode->dirBlockPtr[1];
        else
        {
            int indirect_block[BLOCK_SIZE / sizeof(int)];
            DevReadBlock(file_inode->indirBlockPtr, (char *)indirect_block);
            readBlockNum = indirect_block[(oh_index / BLOCK_SIZE) - 2];
        }
        DevReadBlock(readBlockNum, readBlock);
        int read_index = BLOCK_SIZE - (((oh_index / BLOCK_SIZE + 1) * BLOCK_SIZE) - oh_index);
        for (int i = 0; i < (((oh_index / BLOCK_SIZE + 1) * BLOCK_SIZE) - oh_index); i++)
        {
            pBuffer[totalReadSize] = readBlock[read_index];
            read_index++;
            totalReadSize++;
            if (totalReadSize >= length)
            {
                pFileDescTable->file[fileDesc].fileOffset = totalReadSize + pFileDescTable->file[fileDesc].fileOffset;
                free(readBlock);
                free(file_inode);
                return totalReadSize;
            }
        }
    }
}

int CloseFile(int fileDesc)
{

    if (pFileDescTable->file[fileDesc].bUsed == 1)
    {
        pFileDescTable->file[fileDesc].fileOffset = 0;
        pFileDescTable->file[fileDesc].bUsed = 0;
        pFileDescTable->file[fileDesc].inodeNum = 0;
        return 0;
    }

    return -1;
}

int RemoveFile(const char *szFileName)
{
    //    • 파일을 제거한다. 단, open된 파일을 제거할 수 없다.
    //    • Parameters
    //    ◦ szFileName[in]: 제거할 파일 이름. 단, 파일 이름은 절대 경로임.
    //                                              - Return
    //    성공하면, 0를 리턴한다. 실패했을때는 -1을 리턴한다. 실패 원인으로 (1) 제거할 파일 이름이 없을 경우, (2) 제거될 파일이 open되어 있을 경우.
    Inode *pInode = (Inode *)malloc(sizeof(Inode));

    int curInodeNum = 0;
    int arr_index = 0;
    char arr[64][MAX_NAME_LEN + 1]; //폴더 갯수 최대 그냥 64로 했음
    char *input_path = (char *)malloc(sizeof(szFileName));
    strcpy(input_path, szFileName);
    char *temp_array = strtok(input_path, "/");
    while (temp_array != NULL)
    {
        strcpy(arr[arr_index++], temp_array);
        temp_array = strtok(NULL, "/");
    }
    int oh_array_index = 0;
    GetInode(0, pInode);
    int parentInodeNum = 0;
    for (oh_array_index; oh_array_index < arr_index - 1; oh_array_index++) // 루트에 만들어지면 여기는 그냥 통과가 되버리네
    {
        parentInodeNum = findInode(arr[oh_array_index], pInode); // 이게 돌아주면 될것 같다
    }

    curInodeNum = findInode(arr[oh_array_index], pInode); // 이게 돌아주면 될것 같다

    if (curInodeNum == -1)
    {
        free(pInode);

        return -1;
    }

    for (int i = 0; i < MAX_FD_ENTRY_LEN; i++)
    {
        if (pFileDescTable->file[i].inodeNum == curInodeNum)
        {
            free(pInode);
            return -1;
        }
    }

    Inode *file_inode = (Inode *)malloc(sizeof(Inode));
    GetInode(curInodeNum, file_inode);

    char *pBlock = (char *)malloc(BLOCK_SIZE);
    for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++)
    {

        if (file_inode->dirBlockPtr[i] != 0)
        {
            DevReadBlock(file_inode->dirBlockPtr[i], pBlock);
            memset(pBlock, 0, BLOCK_SIZE);
            DevWriteBlock(file_inode->dirBlockPtr[i], pBlock);
            ResetBlockBitmap(file_inode->dirBlockPtr[i]);
            DevReadBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
            pFileSysInfo->numAllocBlocks--;
            pFileSysInfo->numFreeBlocks++;
            DevWriteBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
            file_inode->dirBlockPtr[i] = 0;
        }
    }

    if (file_inode->indirBlockPtr != 0)
    {

        int indirect_block[BLOCK_SIZE / sizeof(int)];
        DevReadBlock(file_inode->indirBlockPtr, (char *)indirect_block);
        for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++)
        {

            if (indirect_block[i] != 0)
            {

                DevReadBlock(indirect_block[i], pBlock);
                memset(pBlock, 0, BLOCK_SIZE);
                DevWriteBlock(indirect_block[i], pBlock);
                DevReadBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
                pFileSysInfo->numAllocBlocks--;
                pFileSysInfo->numFreeBlocks++;
                DevWriteBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
                ResetBlockBitmap(indirect_block[i]);
            }
        }
        DevReadBlock(file_inode->indirBlockPtr, pBlock);
        memset(pBlock, 0, BLOCK_SIZE);
        DevWriteBlock(file_inode->indirBlockPtr, pBlock);
        ResetBlockBitmap(file_inode->indirBlockPtr);
        DevReadBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
        pFileSysInfo->numAllocBlocks--;
        pFileSysInfo->numFreeBlocks++;
        DevWriteBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
        file_inode->indirBlockPtr = 0;
    }
    ResetInodeBitmap(curInodeNum);
    DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
    pFileSysInfo->numAllocInodes--;
    DevWriteBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
    file_inode->size = 0;
    file_inode->type = 0;

    Inode *parentInode = (Inode *)malloc(BLOCK_SIZE);
    GetInode(parentInodeNum, parentInode);

    DirEntry directory_entry[NUM_OF_DIRENT_PER_BLOCK];
    for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++)
    {

        DevReadBlock(parentInode->dirBlockPtr[i], directory_entry);
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++)
        {
            if (directory_entry[j].inodeNum == curInodeNum)
            {
                directory_entry[j].inodeNum = 0;
                memset(directory_entry[j].name, 0, sizeof(directory_entry[j].name));
                DevWriteBlock(parentInode->dirBlockPtr[i], directory_entry);
                int numOfEmptyBlockInDE = 0;
                for (int i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++)
                {
                    if (directory_entry[i].inodeNum == 0)
                        numOfEmptyBlockInDE++;
                }
                if (numOfEmptyBlockInDE == 4)
                {
                    ResetBlockBitmap(parentInode->dirBlockPtr[i]);
                    DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
                    pFileSysInfo->numAllocBlocks--;
                    pFileSysInfo->numFreeBlocks++;
                    DevWriteBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);

                    parentInode->dirBlockPtr[i] = 0;
                    parentInode->size -= BLOCK_SIZE;
                    PutInode(parentInodeNum, parentInode);
                }
                PutInode(parentInodeNum, parentInode);
                PutInode(curInodeNum, pInode);
                free(pInode);
                free(file_inode);
                free(pBlock);
                free(parentInode);
                return 0;
            }
        }
    }
    int indirect_block[BLOCK_SIZE / sizeof(int)];
    DevReadBlock(parentInode->indirBlockPtr, indirect_block);
    for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++)
    {

        DevReadBlock(indirect_block[i], directory_entry);
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++)
        {
            if (directory_entry[j].inodeNum == curInodeNum)
            {
                directory_entry[j].inodeNum = 0;

                DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
                pFileSysInfo->numAllocInodes--;
                DevWriteBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);

                ResetInodeBitmap(curInodeNum);
                memset(directory_entry[j].name, 0, sizeof(directory_entry[j].name));

                DevWriteBlock(indirect_block[i], directory_entry);

                int numOfEmptyBlockInDE = 0;
                for (int i = 0; i < NUM_OF_DIRENT_PER_BLOCK; i++)
                {
                    if (directory_entry[i].inodeNum == 0)
                        numOfEmptyBlockInDE++;
                }

                if (numOfEmptyBlockInDE == 4)
                {
                    ResetBlockBitmap(indirect_block[i]);
                    DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
                    pFileSysInfo->numAllocBlocks--;
                    pFileSysInfo->numFreeBlocks++;
                    DevWriteBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
                    indirect_block[i] = 0;
                    parentInode->size -= BLOCK_SIZE;
                    DevWriteBlock(parentInode->indirBlockPtr, indirect_block);
                }

                int numIndirectDE = 0;
                for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++)
                {
                    if (indirect_block[i] != 0)
                    {
                        numIndirectDE++;
                    }
                }
                if (numIndirectDE == 0)
                {
                    ResetInodeBitmap(parentInode->indirBlockPtr);

                    DevReadBlock(FILESYS_INFO_BLOCK, pFileSysInfo);
                    pFileSysInfo->numAllocInodes--;
                    DevWriteBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
                    parentInode->indirBlockPtr = 0;
                    PutInode(parentInodeNum, parentInode);
                }

                PutInode(parentInodeNum, parentInode);
                PutInode(curInodeNum, pInode);

                free(pInode);

                free(file_inode);
                free(pBlock);
                free(parentInode);
                return 0;
            }
        }
    }
}

int MakeDir(const char *szDirName)
{
    int arr_index = 0;
    char arr[64][MAX_NAME_LEN + 1]; //폴더 갯수 최대 그냥 64로 했음
    char *input_path = (char *)malloc(sizeof(szDirName));
    strcpy(input_path, szDirName);
    char *temp_array = strtok(input_path, "/");
    while (temp_array != NULL)
    {
        strcpy(arr[arr_index++], temp_array);
        temp_array = strtok(NULL, "/");
    }
    int oh_array_index = 0;
    Inode *pInode = (Inode *)malloc(sizeof(Inode));
    GetInode(0, pInode);
    int parentInodeNum = 0;
    for (oh_array_index; oh_array_index < arr_index - 1; oh_array_index++) // 루트에 만들어지면 여기는 그냥 통과가 되버리네
    {
        parentInodeNum = findInode(arr[oh_array_index], pInode); // 이게 돌아주면 될것 같다
    }
    int block_num; //이거 왜 1이 들어가는거야 시발
    int new_block_num = -1;
    DirEntry directory_entry[NUM_OF_DIRENT_PER_BLOCK];
    memset(directory_entry, 0, BLOCK_SIZE);
    for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++)
    {
        block_num = pInode->dirBlockPtr[i];
        if (pInode->dirBlockPtr[i] == 0)
        {
            new_block_num = GetFreeBlockNum();
            DevReadBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
            pFileSysInfo->numAllocBlocks++;
            pFileSysInfo->numFreeBlocks--;
            DevWriteBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
            SetBlockBitmap(new_block_num);
            DirEntry newDirEntry[NUM_OF_DIRENT_PER_BLOCK];
            memset(newDirEntry, 0, BLOCK_SIZE);
            pInode->dirBlockPtr[i] = new_block_num;
            DevWriteBlock(new_block_num, (char *)newDirEntry);
            pInode->dirBlockPtr[i] = new_block_num;
            pInode->size = (short)(BLOCK_SIZE + pInode->size);
            block_num = pInode->dirBlockPtr[i];
            PutInode(parentInodeNum, pInode);
        }
        DevReadBlock(block_num, (char *)directory_entry);

        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++)
        {
            if (strcmp(directory_entry[j].name, "") == 0)
            {
                for (int k = 0; k < NUM_OF_DIRENT_PER_BLOCK; k++)
                {
                    if (strcmp(directory_entry[k].name, "") == 0)
                    {

                        int new_inode_num;
                        new_inode_num = GetFreeInodeNum();
                        DevReadBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
                        pFileSysInfo->numAllocInodes++;
                        DevWriteBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
                        SetInodeBitmap(new_inode_num);
                        Inode *new_inode;
                        new_inode = (Inode *)malloc(BLOCK_SIZE);
                        new_inode->size = 0;
                        new_inode->type = FILE_TYPE_DIR;

                        PutInode(new_inode_num, new_inode);

                        directory_entry[k].inodeNum = new_inode_num;
                        strcpy(directory_entry[k].name, arr[arr_index - 1]);

                        DevWriteBlock(block_num, (char *)directory_entry);

                        new_block_num = GetFreeBlockNum();
                        SetBlockBitmap(new_block_num);
                        DevReadBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
                        pFileSysInfo->numAllocBlocks++;
                        pFileSysInfo->numFreeBlocks--;
                        DevWriteBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
                        DirEntry new_direct_entry[NUM_OF_DIRENT_PER_BLOCK];
                        memset(new_direct_entry, 0, BLOCK_SIZE);
                        new_direct_entry[0].inodeNum = new_inode_num;
                        strcpy(new_direct_entry[0].name, ".");

                        new_direct_entry[1].inodeNum = parentInodeNum;
                        strcpy(new_direct_entry[1].name, "..");
                        DevWriteBlock(block_num, (char *)directory_entry);

                        DevWriteBlock(new_block_num, (char *)new_direct_entry);

                        new_inode->dirBlockPtr[0] = new_block_num;
                        PutInode(new_inode_num, new_inode);
                        free(new_inode);
                        return 0;
                    }
                }
                return -1;
            }
        }
    }

    if (pInode->indirBlockPtr == 0)
    {
        new_block_num = GetFreeBlockNum();
        DevReadBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
        pFileSysInfo->numAllocBlocks++;
        pFileSysInfo->numFreeBlocks--;
        DevWriteBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
        SetBlockBitmap(new_block_num);

        int indirect_block[BLOCK_SIZE / sizeof(int)];
        memset(indirect_block, 0, BLOCK_SIZE);
        pInode->indirBlockPtr = new_block_num;
        DevWriteBlock(new_block_num, (char *)indirect_block);
        PutInode(parentInodeNum, pInode);
    }

    int indirect_block[BLOCK_SIZE / sizeof(int)];
    DevReadBlock(pInode->indirBlockPtr, (char *)indirect_block);

    for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++)
    {

        if (indirect_block[i] == 0)
        {
            new_block_num = GetFreeBlockNum();
            SetBlockBitmap(new_block_num);
            DevReadBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
            pFileSysInfo->numAllocBlocks++;
            pFileSysInfo->numFreeBlocks--;
            DevWriteBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
            pInode->size = (short)(BLOCK_SIZE + pInode->size);
            DirEntry *newDirEntry = (DirEntry *)malloc(BLOCK_SIZE);
            memset(newDirEntry, 0, BLOCK_SIZE);
            indirect_block[i] = new_block_num;
            DevWriteBlock(pInode->indirBlockPtr, (char *)indirect_block);
            DevWriteBlock(new_block_num, (char *)newDirEntry);
            free(newDirEntry);
        }
        block_num = indirect_block[i];
        DevReadBlock(indirect_block[i], (char *)directory_entry);

        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++)
        {
            if (strcmp(directory_entry[j].name, "") == 0)
            {
                for (int k = 0; k < NUM_OF_DIRENT_PER_BLOCK; k++)
                {
                    if (strcmp(directory_entry[k].name, "") == 0)
                    {
                        int new_inode_num = GetFreeInodeNum();
                        DevReadBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
                        pFileSysInfo->numAllocInodes++;
                        DevWriteBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
                        Inode *new_inode = (Inode *)malloc(BLOCK_SIZE);
                        new_inode->type = FILE_TYPE_DIR;
                        new_inode->size = 0;
                        PutInode(GetFreeInodeNum(), new_inode);
                        strcpy(directory_entry[k].name, arr[arr_index - 1]);
                        directory_entry[k].inodeNum = GetFreeInodeNum();
                        SetInodeBitmap(GetFreeInodeNum());
                        DevWriteBlock(block_num, (char *)directory_entry);
                        DevReadBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
                        pFileSysInfo->numAllocBlocks++;
                        pFileSysInfo->numFreeBlocks--;
                        DevWriteBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
                        DirEntry new_direct_entry[NUM_OF_DIRENT_PER_BLOCK];
                        memset(new_direct_entry, 0, BLOCK_SIZE);
                        strcpy(new_direct_entry[0].name, ".");
                        new_direct_entry[0].inodeNum = new_inode_num;
                        strcpy(new_direct_entry[1].name, "..");
                        new_direct_entry[1].inodeNum = parentInodeNum;
                        DevWriteBlock(GetFreeBlockNum(), (char *)new_direct_entry);
                        DevWriteBlock(block_num, (char *)directory_entry);
                        new_inode->dirBlockPtr[0] = GetFreeBlockNum();
                        SetBlockBitmap(GetFreeBlockNum());
                        PutInode(new_inode_num, new_inode);
                        free(new_inode);
                        DevWriteBlock(pInode->indirBlockPtr, (char *)indirect_block);
                        return 0;
                    }
                }
                return -1;
            }
        }
    }
    return -1;
}

int RemoveDir(const char *szDirName)
{
    int curInodeNum = 0;
    int arr_index = 0;
    char arr[64][MAX_NAME_LEN + 1];
    char *input_path = (char *)malloc(sizeof(szDirName));
    strcpy(input_path, szDirName);
    char *temp_array = strtok(input_path, "/");
    while (temp_array != NULL)
    {
        strcpy(arr[arr_index++], temp_array);
        temp_array = strtok(NULL, "/");
    }
    int oh_array_index = 0;
    Inode *pInode = (Inode *)malloc(sizeof(Inode));
    GetInode(0, pInode);
    int parentInodeNum = 0;
    for (oh_array_index; oh_array_index < arr_index - 1; oh_array_index++)
    {
        parentInodeNum = findInode(arr[oh_array_index], pInode);
    }
    curInodeNum = findInode(arr[oh_array_index], pInode); // 이게 돌아주면 될것 같다
    if (curInodeNum == -1)
        return -1;
    char *pBlock = (char *)malloc(BLOCK_SIZE);
    ResetBlockBitmap(pInode->dirBlockPtr[0]);
    ResetInodeBitmap(curInodeNum);
    DevReadBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
    pFileSysInfo->numAllocInodes--;
    DevWriteBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
    DevReadBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
    pFileSysInfo->numAllocBlocks--;
    pFileSysInfo->numFreeBlocks++;
    DevWriteBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
    Inode *parentInode = (Inode *)malloc(BLOCK_SIZE);
    GetInode(parentInodeNum, parentInode);
    DirEntry directory_entry[NUM_OF_DIRENT_PER_BLOCK];
    for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++)
    {
        DevReadBlock(parentInode->dirBlockPtr[i], (char *)directory_entry);
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++)
        {
            if (directory_entry[j].inodeNum == curInodeNum)
            {
                directory_entry[j].inodeNum = 0;
                memset(directory_entry[j].name, 0, sizeof(directory_entry[j].name));
                DevWriteBlock(parentInode->dirBlockPtr[i], (char *)directory_entry);
                int numOfEmptyBlockInDE = 0;
                for (int k = 0; k < NUM_OF_DIRENT_PER_BLOCK; k++)
                {
                    if (directory_entry[k].inodeNum == 0)
                        numOfEmptyBlockInDE++;
                }
                if (numOfEmptyBlockInDE == 4)
                {
                    ResetBlockBitmap(parentInode->dirBlockPtr[i]);
                    DevReadBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
                    pFileSysInfo->numAllocBlocks--;
                    pFileSysInfo->numFreeBlocks++;
                    DevWriteBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
                    parentInode->dirBlockPtr[i] = 0;
                    parentInode->size -= BLOCK_SIZE;
                    PutInode(parentInodeNum, parentInode);
                }

                return 0;
            }
        }
    }
    int indirect_block[BLOCK_SIZE / sizeof(int)];
    DevReadBlock(parentInode->indirBlockPtr, (char *)indirect_block);
    for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++)
    {
        DevReadBlock(indirect_block[i], (char *)directory_entry);
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++)
        {
            if (directory_entry[j].inodeNum == curInodeNum)
            {
                directory_entry[j].inodeNum = 0;
                DevReadBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
                pFileSysInfo->numAllocInodes--;
                DevWriteBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
                ResetInodeBitmap(curInodeNum);
                memset(directory_entry[j].name, 0, sizeof(directory_entry[j].name));

                DevWriteBlock(indirect_block[i], (char *)directory_entry);

                int numOfEmptyBlockInDE = 0;
                for (int k = 0; k < NUM_OF_DIRENT_PER_BLOCK; k++)
                {
                    if (directory_entry[k].inodeNum == 0)
                        numOfEmptyBlockInDE++;
                }

                if (numOfEmptyBlockInDE == 4)
                {
                    ResetBlockBitmap(indirect_block[i]);
                    DevReadBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
                    pFileSysInfo->numAllocBlocks--;
                    pFileSysInfo->numFreeBlocks++;
                    DevWriteBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
                    indirect_block[i] = 0;
                    parentInode->size -= BLOCK_SIZE;
                    DevWriteBlock(parentInode->indirBlockPtr, (char *)indirect_block);
                }
                int numIndirectDE = 0;
                for (int k = 0; k < BLOCK_SIZE / sizeof(int); k++)
                {
                    if (indirect_block[k] != 0)
                    {
                        numIndirectDE++;
                    }
                }
                if (numIndirectDE == 0)
                {
                    ResetInodeBitmap(parentInode->indirBlockPtr);
                    DevReadBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
                    pFileSysInfo->numAllocInodes--;
                    DevWriteBlock(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
                    parentInode->indirBlockPtr = 0;
                    PutInode(parentInodeNum, parentInode);
                }
                return 0;
            }
        }
    }

    free(pInode);

    free(pBlock);
    free(parentInode);
    return 0;
}

int EnumerateDirStatus(const char *szDirName, DirEntryInfo *pDirEntry, int dirEntrys)
{
    Inode *pInode = (Inode *)malloc(sizeof(Inode));
    int curInodeNum = 0;
    int arr_index = 0;
    char arr[64][MAX_NAME_LEN + 1]; //폴더 갯수 최대 그냥 64로 했음
    char *input_path = (char *)malloc(sizeof(szDirName));
    strcpy(input_path, szDirName);
    char *temp_array = strtok(input_path, "/");
    while (temp_array != NULL)
    {
        strcpy(arr[arr_index++], temp_array);
        temp_array = strtok(NULL, "/");
    }
    int oh_array_index = 0;
    GetInode(0, pInode);
    int parentInodeNum = 0;

    for (oh_array_index; oh_array_index < arr_index - 1; oh_array_index++) // 루트에 만들어지면 여기는 그냥 통과가 되버리네
    {
        parentInodeNum = findInode(arr[oh_array_index], pInode); // 이게 돌아주면 될것 같다
    }
    curInodeNum = findInode(arr[oh_array_index], pInode); // 이게 돌아주면 될것 같다
    if (curInodeNum == -1)
    {
        free(pInode);
        return -1;
    }

    Inode *parentInode = (Inode *)malloc(sizeof(Inode));
    GetInode(parentInodeNum, parentInode);
    int count_index = 0;
    DirEntry directory_entry[NUM_OF_DIRENT_PER_BLOCK];
    for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++)
    {
        if (pInode->dirBlockPtr[i] != 0)
        {
            DevReadBlock(pInode->dirBlockPtr[i], (char *)directory_entry);
            Inode *tempInode = (Inode *)malloc(sizeof(Inode));
            for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++)
            {
                if (count_index >= dirEntrys)
                {
                    free(tempInode);
                    free(pInode);
                    free(parentInode);
                    return count_index;
                }
                if (strcmp(directory_entry[j].name, "") != 0)
                {
                    strcpy(pDirEntry[count_index].name, directory_entry[j].name);
                    pDirEntry[count_index].inodeNum = directory_entry[j].inodeNum;
                    GetInode(directory_entry[j].inodeNum, tempInode);
                    pDirEntry[count_index].type = (FileType)tempInode->type;
                    count_index++;
                }
            }
            free(tempInode);
            if (count_index >= dirEntrys)
            {
                free(pInode);
                free(parentInode);
                return count_index;
            }
        }
    }
    if (pInode->indirBlockPtr != 0)
    {
        int indirect_block[BLOCK_SIZE / sizeof(int)];
        DevReadBlock(pInode->indirBlockPtr, (char *)indirect_block);
        for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++)
        {
            if (indirect_block[i] != 0)
            {

                DevReadBlock(indirect_block[i], (char *)directory_entry);
                Inode *tempInode = (Inode *)malloc(sizeof(Inode));
                for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++)
                {
                    if (count_index >= dirEntrys)
                        break;
                    if (strcmp(directory_entry[j].name, "") != 0)
                    {
                        pDirEntry[count_index].inodeNum = directory_entry[j].inodeNum;
                        strcpy(pDirEntry[count_index].name, directory_entry[j].name);
                        GetInode(directory_entry[j].inodeNum, tempInode);
                        pDirEntry[count_index].type = (FileType)tempInode->type;
                        count_index++;
                    }
                }
                free(tempInode);
                if (count_index >= dirEntrys)
                {
                    free(pInode);
                    free(parentInode);
                    return count_index;
                }
            }
        }
    }
    free(pInode);
    free(parentInode);
    return count_index;
}

void FileSysInit(void) //Success
{
    DevCreateDisk();
    char *buf = malloc(BLOCK_SIZE);
    memset(buf, 0, BLOCK_SIZE);   // memset을 통해서 모든 메모리를 0으로 만듭니다.
    for (int i = 0; i < 512; i++) //512 블록까지 초기화 하라는건가?
    {
        DevWriteBlock(i, buf);
    }
    free(buf);
    if (pFileDescTable == NULL)
    { //초기화
        pFileDescTable = (FileDescTable *)malloc(sizeof(FileDescTable));
        memset(pFileDescTable, 0, sizeof(FileDescTable));
    }
}

void SetInodeBitmap(int inodeno)
{
    char *buf = malloc(BLOCK_SIZE);
    //DevOpenDisk();
    DevReadBlock(INODE_BITMAP_BLOCK_NUM, buf);
    buf[inodeno / 8] |= 1 << (7 - (inodeno % 8));
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
    free(buf); // 왜 세그멘트 나오지
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

    //DevOpenDisk();
    char *buf = malloc(BLOCK_SIZE);
    DevReadBlock((INODELIST_BLOCK_FIRST + inodeno / (BLOCK_SIZE / sizeof(Inode))), buf);
    memcpy(buf + ((inodeno % (BLOCK_SIZE / sizeof(Inode))) * sizeof(Inode)), pInode, sizeof(Inode));
    DevWriteBlock((INODELIST_BLOCK_FIRST + inodeno / (BLOCK_SIZE / sizeof(Inode))), buf);
    free(buf);
}

void GetInode(int inodeno, Inode *pInode)
{

    //DevOpenDisk();

    char *buf = (char *)malloc(BLOCK_SIZE);

    DevReadBlock(INODELIST_BLOCK_FIRST + inodeno / (BLOCK_SIZE / sizeof(Inode)), buf);
    memcpy(pInode, buf + (inodeno % (BLOCK_SIZE / sizeof(Inode)) * sizeof(Inode)), sizeof(Inode));

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
        for (int index_direct = 0; index_direct < 8; index_direct++)
        {
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

int GetFreeBlockNum(void)
{
    char *buf = malloc(BLOCK_SIZE);
    DevReadBlock(BLOCK_BITMAP_BLOCK_NUM, buf);
    int i = 2; // 19부터 하기 위함
    for (int index_direct = 3; index_direct < 8; index_direct++)
    {
        if ((buf[i] << index_direct & 128) == 0) // >> 연산자가 & 보다 우선순위가 높구나
        {
            free(buf);
            return (i * 8) + index_direct;
        }
    }
    for (int i = 3; i < BLOCK_SIZE; i++)
    {
        if (buf[i] == -1) // if 11111111 => continue
            continue;

        for (int index_direct = 0; index_direct < 8; index_direct++)
        {
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