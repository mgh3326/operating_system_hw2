//main 함수가 있는 파일을 만들어서 테스트하기 바랍니다.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "disk.h"
#include "fs.h"

#define DIR_NUM_MAX      100
#define INODE_SIZE 64
#define FILENAME_MAX_LEN 30

int MakeDirtest(const char *szDirName);

void PrintInodeBitmap(void) {
    int i;
    int count;
    int *pBitmap = (int *) malloc(BLOCK_SIZE);

    count = BLOCK_SIZE / sizeof(int);
    DevReadBlock(2, (char *) pBitmap);
    printf("Inode bitmap: ");
    for (i = 0; i < count; i++)
        printf("%d", pBitmap[i]);
    printf("\n");
}

void PrintBlockBitmap(void) {
    int i;
    int count;
    int *pBitmap = (int *) malloc(BLOCK_SIZE);

    count = BLOCK_SIZE / sizeof(int);         /* bit ������ 64*8 ���� ���� */
    DevReadBlock(1, (char *) pBitmap);
    printf("Block bitmap");
    for (i = 0; i < count; i++)
        printf("%d", pBitmap[i]);
    printf("\n");
}

void ListDirContents(const char *dirName) {
    int i;
    int count;
    DirEntryInfo pDirEntry[DIR_NUM_MAX];

    count = EnumerateDirStatus(dirName, pDirEntry, DIR_NUM_MAX);
    printf("[%s]Sub-directory:\n", dirName);
    for (i = 0; i < count; i++) {
        if (pDirEntry[i].type == FILE_TYPE_FILE)
            printf("\t name:%s, inode no:%d, type:file\n", pDirEntry[i].name, pDirEntry[i].inodeNum);
        else if (pDirEntry[i].type == FILE_TYPE_DIR)
            printf("\t name:%s, inode no:%d, type:directory\n", pDirEntry[i].name, pDirEntry[i].inodeNum);
        else {
            assert(0);
        }
    }
}

int main() {
    Mount(MT_TYPE_FORMAT);

    int i;
    char dirName[MAX_NAME_LEN];

    printf(" ---- Test Case 1 ----\n");

    MakeDir("/tmp");
    MakeDir("/usr");//이거부터 안되네
    MakeDir("/etc");
    MakeDir("/home");
    /* make home directory */
    for (i = 0; i < 7; i++) {
        memset(dirName, 0, MAX_NAME_LEN);
        sprintf(dirName, "/home/user%d", i);
        MakeDir(dirName);
    }
//    /* make etc directory */
    for (i = 0; i < 24; i++) {
        memset(dirName, 0, MAX_NAME_LEN);
        sprintf(dirName, "/etc/dev%d", i);
        MakeDir(dirName);
    }
    ListDirContents("/home");
    ListDirContents("/etc");

    /* remove subdirectory of etc directory */
//    for (i = 23; i >= 0; i--) {
//        memset(dirName, 0, MAX_NAME_LEN);
//        sprintf(dirName, "/etc/dev%d", i);
//        RemoveDir(dirName);
//    }
//
    ListDirContents("/etc");

    /* remove subdirectory of root directory except /home */
//    RemoveDir("/etc");
//    RemoveDir("/usr");
//    RemoveDir("/tmp");
    PrintInodeBitmap();
    PrintBlockBitmap();
//    int fd;
//    char fileName[FILENAME_MAX_LEN];
//    char pBuffer1[BLOCK_SIZE];
//
//    printf(" ---- Test Case 2 ----\n");
//
//
//    ListDirContents("/home");
//    /* make home directory */
//    for (i = 0; i < 7; i++) {
//
//        for (int j = 0; j < 7; j++) {
//
//            memset(fileName, 0, FILENAME_MAX_LEN);
//            sprintf(fileName, "/home/user%d/file%d", i, j);
//            fd = OpenFile(fileName, OPEN_FLAG_CREATE);
//            memset(pBuffer1, 0, BLOCK_SIZE);
//            strcpy(pBuffer1, fileName);
//            WriteFile(fd, pBuffer1, BLOCK_SIZE);
//
//            CloseFile(fd);
//        }
//    }
//
//    for (i = 0; i < 7; i++) {
//        memset(dirName, 0, MAX_NAME_LEN);
//        sprintf(dirName, "/home/user%d", i);
//        ListDirContents(dirName);
//    }
//    PrintInodeBitmap();
//    PrintBlockBitmap();
//    ListDirContents("/etc");


}
