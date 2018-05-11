//main 함수가 있는 파일을 만들어서 테스트하기 바랍니다.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disk.h"
#include "fs.h"

#define INODE_SIZE 64

int main() {
    Mount(MT_TYPE_FORMAT);


    MakeDir("/tmp");
    MakeDir("/usr");
    MakeDir("/ect");
    MakeDir("/home");
    MakeDir("/home/user1");
    MakeDir("/home/user2");
    MakeDir("/home/user3");
    MakeDir("/home/user4");
    MakeDir("/home/user5");
    MakeDir("/home/user6");
    MakeDir("/home/user7");

}