#include "fs.h"
#include "disk.h"
//#include "Matrix.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    Mount(MT_TYPE_FORMAT);

    int a = MakeDir("/temp");

//    int b = MakeDir("/temp/a.c");
//    printf("MakeDir(/hi) retrun %d, MakeDir(/hi/bye) return %d\n", a, b);
    printf("----------------------------------------\n");
    int c = OpenFile("/temp/a.c", OPEN_FLAG_CREATE);
    printf("OpenFile(/aa) return %d\n", c);

//    int d = OpenFile("/hi/bb", OPEN_FLAG_CREATE);
//    printf("OpenFile(/hi/bb) return %d\n", d);
//
//    int e = OpenFile("/hi/bye/cc", OPEN_FLAG_CREATE);
//    printf("OpenFile(/hi/bye/cc) return %d\n", e);



    int g = WriteFile(c, "hello", 5);
    printf("WriteFile(c, \"hello\", 5) return %d\n", g);

    char *buffer = (char *) malloc(10);
    int h = ReadFile(c, buffer, 5);
    printf("ReadFile(c, %s, 5) return %d\n", buffer, h);
    int f = CloseFile(c);
    printf("CloseFile(c) return %d\n", f);
//    int i = RemoveFile("/aa");
//    printf("RemoveFile(/aa) return %d\n", i);
//
//    printf("RemoveFile(/hi/bb) return %d\n", RemoveFile("/aa"));
}
