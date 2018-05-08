//main 함수가 있는 파일을 만들어서 테스트하기 바랍니다.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disk.h"
#include "fs.h"

#define INODE_SIZE 64

int main()
{
    Mount(MT_TYPE_FORMAT);

    int a = MakeDir("/hi");

    int b = MakeDir("/hi/bye");
    printf("MakeDir(/hi) retrun %d, MakeDir(/hi/bye) return %d\n", a, b);

    printf("----------------------------------------\n");
    int c = OpenFile("/aa", OPEN_FLAG_CREATE);
    printf("OpenFile(/aa) return %d\n", c);

    int d = OpenFile("/hi/bb", OPEN_FLAG_CREATE);
    printf("OpenFile(/hi/bb) return %d\n", d);

    int e = OpenFile("/hi/bye/cc", OPEN_FLAG_CREATE);
    printf("OpenFile(/hi/bye/cc) return %d\n", e);
    //     printf("\tmain start\n");
    //     Mount(MT_TYPE_FORMAT);
    //     MakeDir("/tmp");
    //     int oh = OpenFile("/tmp/a.c", OPEN_FLAG_CREATE);
    //     printf("OpenFile Return :  %d\n", oh);
    // //    oh = OpenFile("/tmp/b.c", OPEN_FLAG_CREATE);
    // //    printf("OpenFile Return :  %d\n", oh);
    // //    oh = OpenFile("/tmp/c.c", OPEN_FLAG_CREATE);
    // //    printf("OpenFile Return :  %d\n", oh);
    //     int i;
    //     int count;
    //     DirEntryInfo pDirEntryInfo[20];

    //     if ((count = EnumerateDirStatus("/usr/home/kim", pDirEntryInfo, 20)) < 0)
    //         exit(0); // program terminated due to the error return of the function.
    //     printf("count : %d", count);
    //     for (i = 0; i < count; i++)
    //         printf("directory entry : %s, type : %d, inode number : %d\n", pDirEntryInfo[i].name, pDirEntryInfo[i].type,
    //                pDirEntryInfo[i].inodeNum);

    //     Unmount();
    //     printf("\tmain finish\n");
}
//char IsDigitOne(unsigned char *block, int bitIndex);
//
//unsigned char *BlockToBinary(int digitSize, unsigned char *b);
//
//void main(void)
//{
////    printf("%d\ntest============\n",MT_TYPE_READWRITE);
//    DevCreateDisk();
//    FileSysInit();
//    unsigned char *temp;
//
//    temp = (unsigned char *)malloc(BLOCK_SIZE);
//
//    Inode *nd = (Inode *)malloc(sizeof(Inode));
//
//    nd->size = (short) 100000;//overflow 나서 바꿈
//
//    printf("INODE nd = %s\n", BlockToBinary(32, (unsigned char *)nd));
//    printf("IsDigitOne(5) = %c\n", IsDigitOne((unsigned char *)nd, 5));
//    printf("IsDigitOne(6) = %c\n", IsDigitOne((unsigned char *)nd, 6));
//    printf("IsDigitOne(7) = %c\n", IsDigitOne((unsigned char *)nd, 7));
//    printf("IsDigitOne(8) = %c\n", IsDigitOne((unsigned char *)nd, 8));
//    PutInode(2, nd);
//    printf("PutInode(2, nd);\n");
//    nd = (Inode *)malloc(sizeof(Inode));
//    printf("(Init)INODE nd = %s\n", BlockToBinary(32, (unsigned char *)nd));
//    temp = (unsigned char *)malloc(BLOCK_SIZE);
//    DevReadBlock(3, temp);
//    printf("IsDigitOne(5+%d) = %c\n", 8 * INODE_SIZE * 2, IsDigitOne(temp, 5 + 8 * INODE_SIZE * 2));
//    printf("IsDigitOne(6+%d) = %c\n", 8 * INODE_SIZE * 2, IsDigitOne(temp, 6 + 8 * INODE_SIZE * 2));
//    printf("IsDigitOne(7+%d) = %c\n", 8 * INODE_SIZE * 2, IsDigitOne(temp, 7 + 8 * INODE_SIZE * 2));
//    printf("IsDigitOne(8+%d) = %c\n", 8 * INODE_SIZE * 2, IsDigitOne(temp, 8 + 8 * INODE_SIZE * 2));
//    GetInode(2, nd);
//    printf("GetInode(2, nd);\n");
//    printf("INODE nd = %s\n", BlockToBinary(32, (unsigned char *)nd));
//    printf("IsDigitOne(5) = %c\n", IsDigitOne((unsigned char *)nd, 5));
//    printf("IsDigitOne(6) = %c\n", IsDigitOne((unsigned char *)nd, 6));
//    printf("IsDigitOne(7) = %c\n", IsDigitOne((unsigned char *)nd, 7));
//    printf("IsDigitOne(8) = %c\n", IsDigitOne((unsigned char *)nd, 8));
//}
//
//char IsDigitOne(unsigned char *block, int bitIndex)
//{
//    unsigned char resultBit = 0;
//    unsigned char *temp = (unsigned char *)malloc(BLOCK_SIZE);
//    memcpy(temp, block, BLOCK_SIZE);
//    resultBit = (temp[bitIndex / 8] >> (bitIndex % 8)) & 1;
//    return (resultBit == 1) ? '1' : '0';
//}
//
//unsigned char *BlockToBinary(int digitSize, unsigned char *b)
//{
//    unsigned char *s = (unsigned char *)malloc(digitSize + 1);
//    unsigned char *temp = (unsigned char *)malloc(BLOCK_SIZE);
//    int i = 0;
//    memcpy(temp, b, BLOCK_SIZE);
//    s[digitSize] = '\0';
//    for (int count = digitSize - 1; count >= 0; count--)
//    {
//        s[i++] = (((temp[count / 8] >> (count % 8)) & 1) == (unsigned char)0) ? '0' : '1';
//    }
//    return s;
//}