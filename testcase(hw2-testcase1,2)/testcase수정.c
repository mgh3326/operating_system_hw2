#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h>
#include "disk.h"
#include "fs.h"



#define DIR_NUM_MAX      100

char mode[10] = "";

void PrintInodeBitmap(void)
{
	int i;
	int count;
	int* pBitmap = (int*)malloc(BLOCK_SIZE);

	count = BLOCK_SIZE / sizeof(int);
	DevReadBlock(2, pBitmap);
	printf("Inode bitmap: ");
	for (i = 0; i < count; i++)
		printf("%d", pBitmap[i]);
	printf("\n");
}

void PrintBlockBitmap(void)
{
	int i;
	int count;
	int* pBitmap = (int*)malloc(BLOCK_SIZE);

	count = BLOCK_SIZE / sizeof(int);         /* bit 개수는 64*8 개가 존재 */
	DevReadBlock(1, pBitmap);
	printf("Block bitmap");
	for (i = 0; i < count; i++)
		printf("%d", pBitmap[i]);
	printf("\n");
}

void ReadInode(Inode* pInode, int inodeNo)
{
	char* pBuf = NULL;
	Inode* pMem = NULL;
	int block = pFileSysInfo->inodeListBlock + inodeNo / NUM_OF_INODE_PER_BLOCK;
	int inode = inodeNo % NUM_OF_INODE_PER_BLOCK;

	pBuf = (char*)malloc(BLOCK_SIZE);

	DevReadBlock(block, pBuf);
	pMem = (Inode*)pBuf;
	memcpy(pInode, &pMem[inode], sizeof(Inode));
}

void ListDirContentsAndSize(const char* dirName)
{
	int i;
	int count;
	DirEntryInfo pDirEntry[DIR_NUM_MAX];
	Inode pInode;

	count = EnumerateDirStatus(dirName, pDirEntry, DIR_NUM_MAX);

	printf("[%s]Sub-directory:\n", dirName);
	for (i = 0; i < count; i++)
	{
		if (pDirEntry[i].type == FILE_TYPE_FILE) {
			GetInode(pDirEntry[i].inodeNum, &pInode);
			printf("\t name:%s, inode no:%d, type:file, size:%d, blocks:%d\n", pDirEntry[i].name, pDirEntry[i].inodeNum, pInode.size);
		}
		else if (pDirEntry[i].type == FILE_TYPE_DIR)
			printf("\t name:%s, inode no:%d, type:directory\n", pDirEntry[i].name, pDirEntry[i].inodeNum);
		else
		{
			assert(0);
		}
	}
}
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


void TestCase1(void)
{
	int i;
	char dirName[MAX_NAME_LEN];   

	printf(" ---- Test Case 1 ----\n");

	MakeDir("/tmp");
	MakeDir("/usr");
	MakeDir("/etc");
	MakeDir("/home");
	/* make home directory */
	for (i = 0; i < 8; i++)
	{
		memset(dirName, 0, MAX_NAME_LEN);
		sprintf(dirName, "/home/user%d", i);
		MakeDir(dirName);
	}
	/* make etc directory */
	for (i = 0; i < 24; i++)
	{
		memset(dirName, 0, MAX_NAME_LEN);
		sprintf(dirName, "/etc/dev%d", i);
		MakeDir(dirName);
	}
	ListDirContents("/home");
	ListDirContents("/etc");

	/* remove subdirectory of etc directory */
	for (i = 23; i >= 0; i--)
	{
		memset(dirName, 0, MAX_NAME_LEN);
		sprintf(dirName, "/etc/dev%d", i);
		RemoveDir(dirName);
	}

	ListDirContents("/etc");
}


void TestCase2(void)
{
	int i, j;
	int fd;
	char fileName[MAX_NAME_LEN];
	char dirName[MAX_NAME_LEN];
	char pBuffer1[BLOCK_SIZE];

	printf(" ---- Test Case 2 ----\n");

	if (strcmp(mode, "format") == 0)
		MakeDir("/home");

	ListDirContents("/home");
	/* make home directory */
	for (i = 0; i < 7; i++)
	{

		if (strcmp(mode, "format") == 0) {
			memset(fileName, 0, MAX_NAME_LEN);
			sprintf(fileName, "/home/user%d", i);
			MakeDir(fileName);
		}

		for (j = 0; j < 7; j++)
		{

			memset(fileName, 0, MAX_NAME_LEN);
			sprintf(fileName, "/home/user%d/file%d", i, j);
			fd = OpenFile(fileName, OPEN_FLAG_CREATE);
			memset(pBuffer1, 0, BLOCK_SIZE);
			strcpy(pBuffer1, fileName);
			WriteFile(fd, pBuffer1, BLOCK_SIZE);

			CloseFile(fd);
		}
	}

	for (i = 0; i < 7; i++)
	{
		memset(dirName, 0, MAX_NAME_LEN);
		sprintf(dirName, "/home/user%d", i);
		ListDirContents(dirName);
	}
}

int main(int argc, char** argv)
{
	int TcNum;
	strcpy(mode, argv[1]);
	if (argc < 3)
	{
	ERROR:
		printf("usage: a.out [format | readwrite] [1-2])\n");	//테스트케이스 추가시 변경 사항
		return -1;
	}
	if (strcmp(argv[1], "format") == 0)
		Mount(MT_TYPE_FORMAT);
	else if (strcmp(argv[1], "readwrite") == 0)
		Mount(MT_TYPE_READWRITE);
	else
		goto ERROR;

	TcNum = atoi(argv[2]);


	switch (TcNum)
	{
	case 1:
		TestCase1();
		PrintInodeBitmap(); PrintBlockBitmap();
		break;
	case 2:
		TestCase2();
		PrintInodeBitmap(); PrintBlockBitmap();
		break;
		
	default:
		Unmount();
		goto ERROR;
	}
	Unmount();


	return 0;
}