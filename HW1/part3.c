#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
int my_dup(int oldfd) {
    int newfd = fcntl(oldfd, F_DUPFD, 0);
    if (newfd == -1) {
        return -1;
    }
    return newfd;
}
int main() {
    int fd1, fd2, read_fd;
    off_t offset1, offset2;
    struct stat st1, st2;
    read_fd = open("testfile.txt", O_WRONLY | O_CREAT);
    if (read_fd == -1) {
        perror("open");
        return 1;
    }
    write(read_fd, "bahadir-etka-kilinc",19);
    close(read_fd);
    // Open a file for reading
    fd1 = open("testfile.txt", O_RDONLY);
    if (fd1 == -1) {
        perror("open");
        return 1;
    }
    // Duplicate the file descriptor
    fd2 = my_dup(fd1);
    if (fd2 == -1) {
        perror("dup");
        close(fd1);
        return 1;
    }
    // Read from fd1 and fd2
    char buffer1[12], buffer2[7];
    ssize_t n1 = read(fd1, buffer1, sizeof(buffer1));
    if (n1 == -1) {
        perror("read(fd1)");
        close(fd1);
        close(fd2);
        return 1;
    }
    ssize_t n2 = read(fd2, buffer2, sizeof(buffer2));
    if (n2 == -1) {
        perror("read(fd2)");
        close(fd1);
        close(fd2);
        return 1;
    }
    // Print the contents of the buffers
    printf("Buffer 1: %.*s\n", (int)n1, buffer1);
    printf("Buffer 2: %.*s\n", (int)n2, buffer2);
    // Get the file offsets
    offset1 = lseek(fd1, 0, SEEK_CUR);
    offset2 = lseek(fd2, 0, SEEK_CUR);
    if (offset1 == -1 || offset2 == -1) {
        perror("lseek");
        close(fd1);
        close(fd2);
        return 1;
    }
    // Print the file offsets
    printf("File offset 1: %lld\n", (long long)offset1);
    printf("File offset 2: %lld\n", (long long)offset2);
    // Get the file information
    if (fstat(fd1, &st1) == -1 || fstat(fd2, &st2) == -1) {
        perror("fstat");
        close(fd1);
        close(fd2);
        return 1;
    }
    // Print the file information
    printf("Inode number 1: %llu\n", (unsigned long long)st1.st_ino);
    printf("Inode number 2: %llu\n", (unsigned long long)st2.st_ino);
    // Close the file descriptors
    close(fd1);
    close(fd2);
    return 0;
}
