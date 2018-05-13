//-Mount.c는 Mount, unmount 함수를 구현 합니다.
#include <stdio.h>
#include <stdlib.h>
#include "fs.h"
#include "disk.h"
#include <math.h>
#include <string.h>

FileSysInfo *pFileSysInfo = NULL;

void Mount(MountType type) {
    if (type == MT_TYPE_FORMAT) {
        //        파일 시스템을 포맷하고 초기화하는 동작을 수행한다.
        //                가상 디스크를 초기화, 즉 생성 후에 동작이 다음 쪽부터 설명될 동작을 하도록 구현한다.
        FileSysInit(); //(0)파일시스템 초기화 단계기 때문에 FileSys()을 통해 Block0부터 Block511까지
        // for (int i = 0; i < 19; i++) {// 18번까지 블록 비트맵은 1이 되는것이 맞는것 같다.
        //     SetBlockBitmap(i);
        // }
        int first_block = GetFreeBlockNum();
        int first_inode = GetFreeInodeNum();
        //초기화를 해야 한다.
        //        MakeDir("root");// 루트는 따로 안만드는건가
        pFileSysInfo = malloc(sizeof *pFileSysInfo); // 이렇게 할당 malloc 해주면 되는건가
        pFileSysInfo->blocks = 512;
        pFileSysInfo->rootInodeNum = first_inode; //0맞지?
        pFileSysInfo->diskCapacity = FS_DISK_CAPACITY;
        pFileSysInfo->numAllocBlocks = 0;
        pFileSysInfo->numFreeBlocks = 512 - 19; //-19필요한가
        pFileSysInfo->numAllocInodes = 0;
        pFileSysInfo->blockBitmapBlock = BLOCK_BITMAP_BLOCK_NUM;
        pFileSysInfo->inodeBitmapBlock = INODE_BITMAP_BLOCK_NUM;
        pFileSysInfo->inodeListBlock = INODELIST_BLOCK_FIRST;
        pFileSysInfo->dataRegionBlock = 19;
        //        int num = GetFreeBlockNum() + 19;// 이렇게 해서 19가 나오게 하는게 맞나
        DirEntry *pDirEntry = (DirEntry *) malloc(BLOCK_SIZE);

        // DirEntry *pDirEntry[4]; // 크기가 4인 구조체 포인터 배열 선언
        // // 구조체 포인터 배열 전체 크기에서 요소(구조체 포인터)의 크기로 나눠서 요소 개수를 구함
        // for (int i = 0; i < sizeof(pDirEntry) / sizeof(DirEntry *); i++) // 요소 개수만큼 반복
        // {
        //     pDirEntry[i] = malloc(sizeof(DirEntry)); // 각 요소에 구조체 크기만큼 메모리 할당
        // }                                            //https://dojang.io/mod/page/view.php?id=447 여기서 보고 따라함
        pDirEntry[0].inodeNum = first_inode; //된다 된다
        SetInodeBitmap(first_inode);
        strncpy(pDirEntry[0].name, ".", sizeof(pDirEntry[0].name) - 1); //strcpy는 안좋다니까 strncpy로 함
        DevWriteBlock(first_block, (char *) pDirEntry);                  //이러면 전달 될라나
        SetBlockBitmap(first_block);                                    //19번
        char *buf = malloc(BLOCK_SIZE);                                 //(7) Block 크기의 메모리 할당

        //(8) FileSysInfo으로 형 변환함.
        //디렉토리 한 개 할당,
        //블록 한 개 할당하기 때문에
        //FileSysInfo를 변경함
        pFileSysInfo->numAllocBlocks++;
        pFileSysInfo->numFreeBlocks--;
        pFileSysInfo->numAllocInodes++;
        //        DirEntry* p = (DirEntry*)pBuf;
        FileSysInfo *pbuf = (FileSysInfo *) buf; //형변환 완료 오예
        pbuf = pFileSysInfo;

        //(9) 해당 블록을 DevWriteBlock를
        //사용하여 Block 0에 저장
        DevWriteBlock(0, (char *) pbuf); //이러면 전달 되겠구만
        //DevWriteBlock(0, (char *) pFileSysInfo);//형변환 없이 이렇게 해도 되는건가
        //> Root 디렉토리의 inode를 획득한다.
        Inode *pInode = NULL;
        pInode = malloc(sizeof *pInode); // 이렇게 할당 malloc 해주면 되는건가

        GetInode(first_inode, pInode);
        pInode->dirBlockPtr[0] = first_block;
        pInode->size = 0;
        pInode->type = FILE_TYPE_DIR;
        pInode->indirBlockPtr = 0;

        //> pInode에서 direct block pointer[0]의 값을 block 19로 설정

        //> 변경된 pInode의 값을 디스크에 저장함
        PutInode(first_inode, pInode);
        //        free(pFile/SysInfo);//free 해주면 안되네
        //        free(pDirEntry);
        //        free(buf);
        //        free(pInode);
    } else if (type == MT_TYPE_READWRITE) {
        DevOpenDisk();
        if (pFileDescTable == NULL) {
            pFileDescTable = (FileDescTable *) malloc(sizeof(FileDescTable));
            memset(pFileDescTable, 0, sizeof(FileDescTable));
        }
        if (pFileSysInfo == NULL) {
            pFileSysInfo = (FileSysInfo *) malloc(BLOCK_SIZE);
            DevReadBlock(FILESYS_INFO_BLOCK, (char *) pFileSysInfo);
        }
        //        파일 시스템을 포맷이 아닌, 전원을 켰을 때 파일시스템 사용에 앞서 이루어지는 동작. 실제 파일시스템에서는 복잡한 동작이 이루어진다.
        //                포맷 대신 가상 디스크를 open하는 동작만 수행한다.
    }

}

void Unmount(void) {
    //    전원을 끌 때 호출되는 함수라고 간주하면 된다.
    //            가상 디스크 파일을 close한다.
    DevCloseDisk();
    // printf("do close\n");
}

// // Unmount()
// // 전원을 끌 때 호출되는 함수라고 간주하면 된다.
// // 가상 디스크 파일을 close한다.
