#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>

#define SHARED_MEMORY_SIZE 256

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <shared_memory_name>\n", argv[0]);
        return 1;
    }

    const char *shared_memory_name = argv[1];
    int fd = shm_open(shared_memory_name, O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        return 1;
    }

    char *shared_memory = mmap(NULL, SHARED_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared_memory == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    char *read_msg = shared_memory;
    char *response_msg = shared_memory + 128;

    while (strlen(read_msg) == 0) {
        sleep(1);
    }

    FILE *fp = fopen(read_msg, "w");
    if (!fp) {
        perror("file error");
        return -1;
    }

    while (1) {
        sleep(1);
        if (strncmp(response_msg, "exit", 4) == 0) {
            break;
        }
        int len = strlen(read_msg);
        if (len > 0 && (read_msg[len - 2] == ';' || read_msg[len - 2] == '.')) {
            fputs(read_msg, fp);
            strcpy(response_msg, "Success");
        } else {
            strcpy(response_msg, "Not over in ';' or '.'");
        }
    }
    fclose(fp);

    return 0;
}