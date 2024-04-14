#include <fcntl.h>
#include <stdio.h>

int main(void){

    int fd = open("/dev/chardev", O_RDWR, 0);

    if(fd == -1){
        printf("couldn't open \n");
        return -1;
    }

    char buff[] = {2, 1, 18, 125, 17, 8};
    int written = write(fd,buff,6);
    printf("written: %d\n", written);
    unsigned char res[10];
    int n_read = read(fd,res, 10);

    if(n_read != 10){
        printf("couldn't read");
        return -1;
    }

    for(int i = 0; i < 10; i++){
        printf("%d ",res[i]);
    }

    printf("\n");
    printf("trying to close \n");
    close(fd);

    fd = open("/dev/chardev", O_RDWR, 0);
    unsigned char buff2[] = {3, 1, 18, 19, 125, 17, 8, 48};
    written = write(fd,buff2,8);
    char res2[17];
    n_read = read(fd,res2,17);

    for(int i = 0; i < 17; i++){
        printf("%d ", res2[i]);
    }
    printf("\n");
    close(fd);
    return 0;
}