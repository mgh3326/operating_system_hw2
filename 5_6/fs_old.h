//-fs.h: 과제2에서 구현할 함수 프로토타입, 과제1에서 구현한 함수 프로토타입, disk layout 관련 블록들의 위치가 정의되어 있습니다. 물론, inode, direntry, file decriptor table, file object, filesysinfo 등이 정의되었습니다.
#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include "Disk.h"

// ------- Caution -----------------------------------------
#define FS_DISK_CAPACITY (8388608) /* 8M */
#define MAX_FD_ENTRY_LEN (64)

#define NUM_OF_INODE_PER_BLK (BLOCK_SIZE / sizeof(Inode))
#define NUM_OF_DIRENT_PER_BLK (BLOCK_SIZE / sizeof(DirEntry))

#define NUM_OF_DIRECT_BLK_PTR (2)
#define MAX_INDEX_OF_DIRBLK (NUM_OF_DIRECT_PER_BLK)
#define MAX_NAME_LEN (12)

#define FILESYS_INFO_BLOCK (0)   /* file system info block no. */
#define BLOCK_BITMAP_BLK_NUM (1) /* block bitmap block no. */
#define INODE_BITMAP_BLK_NUM (2) /* inode bitmap block no. */
#define INODELIST_BLK_FIRST (3)  /* the first block no. of inode list */
#define INODELIST_BLKS (16)      /* the number of blocks in inode list */

// ----------------------------------------------------------

typedef enum __openFlag {
    OPEN_FLAG_READWRITE, // 파일을 read/write permission으로 open함. 본 과제에서는 이 flag가 입력되었을 때, 리눅스와 달리 open file table 또는 file descriptor table에 셋팅을 하지 않음. 단순히 파일 open을 수행함

    OPEN_FLAG_CREATE // 파일이 존재하지 않으면 생성 후 파일을 open함
} OpenFlag;

typedef enum __fileType {
    FILE_TYPE_FILE,
    FILE_TYPE_DIR,
    FILE_TYPE_DEV
} FileType;

typedef enum __fileMode {
    FILE_MODE_READONLY,
    FILE_MODE_READWRITE,
    FILE_MODE_EXEC
} FileMode;

typedef struct __dirEntry {
    char name[MAX_NAME_LEN]; // file name
    int inodeNum;
} DirEntry;

typedef enum __mountType {
    MT_TYPE_FORMAT,
    MT_TYPE_READWRITE
} MountType;

typedef struct _FileSysInfo {
    int blocks;           // 디스크에 저장된 전체 블록 개수
    int rootInodeNum;     // 루트 inode의 번호
    int diskCapacity;     // 디스크 용량 (Byte 단위)
    int numAllocBlocks;   // 파일 또는 디렉토리에 할당된 블록 개수
    int numFreeBlocks;    // 할당되지 않은 블록 개수
    int numAllocInodes;   // 할당된 inode 개수
    int blockBitmapBlock; // block bitmap의 시작 블록 번호
    int inodeBitmapBlock; // inode bitmap의 시작 블록 번호
    int inodeListBlock;   // inode list의 시작 블록 번호
    int dataReionBlock;   // data region의 시작 블록 번호
} FileSysInfo;

typedef struct _Inode {
    short size;                           // 파일 크기
    short type;                           // 파일 타입
    int dirBlkPtr[NUM_OF_DIRECT_BLK_PTR]; // Direct block pointers
    int indirBlkPointer;                  // Single indirect block pointer
} Inode;

typedef struct __fileDesc {
    int bUsed;
    int fileOffset;
    int inodeNum;
} FileDesc;

typedef struct __fileDescTable {
    FileDesc file[MAX_FD_ENTRY_LEN];
} FileDescTable;

extern int OpenFile(const char *szFileName, OpenFlag flag);

extern int WriteFile(int fileDesc, char *pBuffer, int length);

extern int ReadFile(int fileDesc, char *pBuffer, int length);

extern int CloseFile(int fileDesc);

extern int RemoveFile(const char *szFileName);

extern int MakeDir(const char *szDirName);

extern int RemoveDir(const char *szDirName);

extern void EnumerateDirStatus(const char *szDirName, DirEntry *pDirEntry, int *pNum);

extern void Mount(MountType type);

extern void Unmount(void);

extern FileDescTable *pFileDescTable;
extern FileSysInfo *pFileSysInfo;

/*  File system internal functions */

void FileSysInit(void);

void SetInodeBitmap(int inodeno);

void ResetInodeBitmap(int inodeno);

void SetBlockBitmap(int blkno);

void ResetBlockBitmap(int blkno);

void PutInode(int inodeno, Inode *pInode);

void GetInode(int inodeno, Inode *pInode);

int GetFreeInodeNum(void);

int GetFreeBlockNum(void);

#endif /* FILESYSTEM_H_ */
