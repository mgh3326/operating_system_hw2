//-Disk.h 수정: block 크기를 16으로 변경. Disk.c는 과제1의 것을 사용하면 됩니다.
#ifndef __DISK_H__
#define __DISK_H__

#define BLOCK_SIZE (64)

extern void DevCreateDisk(void);

extern void DevOpenDisk(void);

extern void DevReadBlock(int blkno, char *pBuf);

extern void DevWriteBlock(int blkno, char *pBuf);
extern void DevCloseDisk(void); //내가 임의로 추가함
#endif /* __DISK_H__ */
