#include <stdio.h>
#include <stdlib.h>
#include "Disk.h"
#include "fs.h"


FileDescTable* pFileDescTable = NULL;


int		OpenFile(const char* szFileName, OpenFlag flag)
{

}


int		WriteFile(int fileDesc, char* pBuffer, int length)
{

}

int		ReadFile(int fileDesc, char* pBuffer, int length)
{

}


int		CloseFile(int fileDesc)
{

}

int		RemoveFile(const char* szFileName)
{

}


int		MakeDir(const char* szDirName)
{

}


int		RemoveDir(const char* szDirName)
{

}


void		EnumerateDirStatus(const char* szDirName, DirEntry* pDirEntry, int* pNum)
{

}
