#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 4) {
        printf("Usage: %s filename num-bytes [x]\n", argv[0]);
        return 1;
    }

    char *filename = argv[1];
    int num_bytes = atoi(argv[2]);
    int use_lseek = (argc == 4);
    int flags = use_lseek ? O_WRONLY : O_WRONLY | O_APPEND;
    int fd = open(filename, flags | O_CREAT, 0644);
    if (fd == -1) {
        printf("Error opening file %s\n", filename);
        return 1;
    }
    int i;
    char c = 'a';
    for (i = 0; i < num_bytes; i++) {
        if (use_lseek) {
            if (lseek(fd, 0, SEEK_END) == -1) {
                printf("Error seeking end of file %s\n", filename);
                close(fd);
                return 1;
            }
        }
        if (write(fd, &c, 1) != 1) {
            printf("Error writing to file %s\n", filename);
            close(fd);
            return 1;
        }
    }
    close(fd);
    return 0;
}