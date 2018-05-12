//main 함수가 있는 파일을 만들어서 테스트하기 바랍니다.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "disk.h"
#include "fs.h"

#define DIR_NUM_MAX      100
#define INODE_SIZE 64

int MakeDirtest(const char *szDirName);
void ListDirContents(const char* dirName)
{
    int i;
    int count;
    DirEntryInfo pDirEntry[DIR_NUM_MAX];

    count = EnumerateDirStatus(dirName, pDirEntry, DIR_NUM_MAX);
    printf("[%s]Sub-directory:\n", dirName);
    for (i = 0; i < count; i++)
    {
        if (pDirEntry[i].type == FILE_TYPE_FILE)
            printf("\t name:%s, inode no:%d, type:file\n", pDirEntry[i].name, pDirEntry[i].inodeNum);
        else if (pDirEntry[i].type == FILE_TYPE_DIR)
            printf("\t name:%s, inode no:%d, type:directory\n", pDirEntry[i].name, pDirEntry[i].inodeNum);
        else
        {
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
    MakeDir("/usr");
    MakeDir("/etc");
    MakeDir("/home");
    /* make home directory */
    for (i = 0; i < 7; i++)
    {
        memset(dirName, 0, MAX_NAME_LEN);
        sprintf(dirName, "/home/user%d", i);
        MakeDir(dirName);
    }
    for (i = 0; i < 24; i++)
    {
        memset(dirName, 0, MAX_NAME_LEN);
        sprintf(dirName, "/etc/dev%d", i);
        MakeDir(dirName);
    }
    ListDirContents("/home");
    ListDirContents("/etc");
//    MakeDir("/home/user1");

//    Inode *pInode = NULL;
//    pInode = malloc(sizeof *pInode); // 이렇게 할당 malloc 해주면 되는건가
//    GetInode(0, pInode); //이게 뭐지
//            for(int i=0;i<6;i++)
//            {
//                GetInode(i, pInode); //이게 뭐지
//                printf("%d\t%d\n",pInode->dirBlockPtr[0],pInode->dirBlockPtr[1]);
//            }


}