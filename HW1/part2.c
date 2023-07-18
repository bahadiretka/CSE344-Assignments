#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
int my_dup(int oldfd) {
    int newfd = fcntl(oldfd, F_DUPFD, 0);
    if (newfd == -1) {
        return -1;
    }
    return newfd;
}
int my_dup2(int oldfd, int newfd) {
    if (oldfd == newfd) {
        // Check if oldfd is valid
        int flags = fcntl(oldfd, F_GETFL);
        if (flags == -1) {
            errno = EBADF;
            return -1;
        }
        return newfd;
    }
    // Close newfd if it's already open
    if (fcntl(newfd, F_GETFL) != -1) {
        close(newfd);
    }
    int result = fcntl(oldfd, F_DUPFD, newfd);
    if (result == -1) {
        return -1;
    }
    return newfd;
}
int main() {
    int fd1, fd2, fd3;
    fd1 = open("file.txt", O_CREAT | O_RDWR, 0666);
    if (fd1 == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    fd2 = my_dup(fd1);
    if (fd2 == -1) {
        perror("my_dup");
        exit(EXIT_FAILURE);
    }
    fd3 = my_dup2(fd1, 5);
    if (fd3 == -1) {
        perror("my_dup2");
        exit(EXIT_FAILURE);
    }
    printf("fd1=%d, fd2=%d, fd3=%d\n", fd1, fd2, fd3);

    write(fd1, "bahadir ", 8);
    write(fd2, "etka ", 5);
    write(fd3, "kilinc", 6);

    lseek(fd1, 0, SEEK_SET);
    char buf[20];
    read(fd1, buf, sizeof(buf));
    printf("%s\n", buf);

    close(fd1);
    close(fd2);
    close(fd3);

    return 0;
}


