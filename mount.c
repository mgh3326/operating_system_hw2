//-Mount.c는 Mount, unmount 함수를 구현 합니다.
#include <stdio.h>
#include <stdlib.h>
#include "fs.h"
#include "Disk.h"
#include <math.h>

FileSysInfo *pFileSysInfo = NULL;

void Mount(MountType type) {
    if (type == MT_TYPE_FORMAT) {
        //        파일 시스템을 포맷하고 초기화하는 동작을 수행한다.
        //                가상 디스크를 초기화, 즉 생성 후에 동작이 다음 쪽부터 설명될 동작을 하도록 구현한다.
        FileSysInit();
        pFileSysInfo = malloc(sizeof *pFileSysInfo); // 이렇게 할당 malloc 해주면 되는건가
        pFileSysInfo->blocks = 512;
        pFileSysInfo->rootInodeNum = 0;
        pFileSysInfo->diskCapacity = 32;
        pFileSysInfo->numAllocBlocks = 0;
        pFileSysInfo->numFreeBlocks = 512 - 19;//-19필요한가
        pFileSysInfo->numAllocInodes = 0;
        pFileSysInfo->blockBitmapBlock = BLOCK_BITMAP_BLK_NUM;
        pFileSysInfo->inodeBitmapBlock = INODE_BITMAP_BLK_NUM;
        pFileSysInfo->inodeListBlock = INODELIST_BLK_FIRST;
        pFileSysInfo->dataReionBlock = (int) (pow(2, 12) - 19);
    } else if (type == MT_TYPE_READWRITE) {
        DevOpenDisk();
        //        파일 시스템을 포맷이 아닌, 전원을 켰을 때 파일시스템 사용에 앞서 이루어지는 동작. 실제 파일시스템에서는 복잡한 동작이 이루어진다.
        //                포맷 대신 가상 디스크를 open하는 동작만 수행한다.
    } else //이건 예외인데 이럴일이 없음.
    {
    }
}

void Unmount(void) {
    //    전원을 끌 때 호출되는 함수라고 간주하면 된다.
    //            가상 디스크 파일을 close한다.
    // printf("do close\n");
    DevCloseDisk(); //이렇게 추가 하면 될라나?
}

// // Unmount()
// // 전원을 끌 때 호출되는 함수라고 간주하면 된다.
// // 가상 디스크 파일을 close한다.
