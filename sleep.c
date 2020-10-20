#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc,char *argv[]){
    int bedtime;
    if(argc<2){
        printf("Please input the sleeping time!\n");
        exit(0);
    }

    bedtime=atoi(argv[1]);
    if(bedtime>0){
        sleep(bedtime);
    }
    else{
        printf("Invalid sleeping time %s\n",argv[1]);
    }
    exit(0);
}