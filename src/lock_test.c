#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int fd = open("data/accounts.dat", O_RDWR);
    if (fd < 0) { perror("open"); return 1; }

    struct flock lock;
    lock.l_type = F_WRLCK;    // write lock
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;           // lock entire file
    printf("Trying to acquire lock...\n");

    fcntl(fd, F_SETLKW, &lock);
    printf("Lock acquired. Sleeping 10s...\n");
    sleep(10);

    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    printf("Lock released.\n");

    close(fd);
    return 0;
}

