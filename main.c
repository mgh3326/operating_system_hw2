#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h>
#include "disk.h"
#include "fs.h"


#define FILENAME_MAX_LEN 30
#define DIR_NUM_MAX      100

int main() {
    Mount(MT_TYPE_FORMAT);

    int i, j, k;
    char alphabet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$^&*()_";
    char fileName[FILENAME_MAX_LEN];
    char *pBuffer1 = (char *) malloc(BLOCK_SIZE);
    char *pBuffer2 = (char *) malloc(BLOCK_SIZE);
    int cIndex = 0;
    int cmpCount = 0;
    int fd[4] = {0,};
    MakeDir("/home");
    Unmount();

    Mount(MT_TYPE_READWRITE);

    MakeDir("/home/test");
    for (i = 0; i < 4; i++) {
        memset(fileName, 0, FILENAME_MAX_LEN);
        sprintf(fileName, "/home/test/file%d", i);
        fd[i] = OpenFile(fileName, OPEN_FLAG_CREATE);
    }

    for (i = 0; i < 18; i++) {
        for (j = 0; j < 4; j++) {
            char *str = (char *) malloc(BLOCK_SIZE);
            memset(str, 0, BLOCK_SIZE);
            for (k = 0; k < BLOCK_SIZE; k++)
                str[k] = alphabet[cIndex];
            WriteFile(fd[j], str, BLOCK_SIZE);
            cIndex++;
            free(str);
        }
    }

    for (i = 0; i < 4; i++)
        CloseFile(fd[i]);


    for (i = 0; i < 4; i++) {
        memset(fileName, 0, FILENAME_MAX_LEN);
        sprintf(fileName, "/home/test/file%d", i);
        fd[i] = OpenFile(fileName, OPEN_FLAG_READWRITE);
    }

    cIndex = 0;

    for (i = 0; i < 18; i++) {
        for (j = 0; j < 4; j++) {
            memset(pBuffer1, 0, BLOCK_SIZE);

            for (k = 0; k < BLOCK_SIZE; k++)
                pBuffer1[k] = alphabet[cIndex];

            memset(pBuffer2, 0, BLOCK_SIZE);
            ReadFile(fd[j], pBuffer2, BLOCK_SIZE);
            if (strcmp(pBuffer1, pBuffer2) == 0)
                cmpCount++;
            else {
                printf("TestCase 3 : error!!\n");
                exit(0);
            }
            cIndex++;
        }
    }
    if (cmpCount == 72)
        printf("TestCase3 : Complete!!!\n");
}
