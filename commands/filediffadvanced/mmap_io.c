#include "mmap_io.h"
#include "util.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

int mmap_file(const char *path, mapped_file_t *mf) {
    mf->path = path;
    mf->data = NULL;
    mf->size = 0;
    mf->fd = -1;

    mf->fd = open(path, O_RDONLY);
    if (mf->fd == -1) {
        perror(path);
        return -1;
    }

    struct stat st;
    if (fstat(mf->fd, &st) == -1) {
        perror("fstat");
        close(mf->fd);
        mf->fd = -1;
        return -1;
    }

    mf->size = (size_t)st.st_size;

    if (mf->size == 0) {
        return 0;
    }

    mf->data = mmap(NULL, mf->size, PROT_READ, MAP_PRIVATE, mf->fd, 0);
    if (mf->data == MAP_FAILED) {
        perror("mmap");
        mf->data = NULL;
        close(mf->fd);
        mf->fd = -1;
        return -1;
    }

#ifdef POSIX_MADV_SEQUENTIAL
    posix_madvise(mf->data, mf->size, POSIX_MADV_SEQUENTIAL);
#elif defined(MADV_SEQUENTIAL)
    madvise(mf->data, mf->size, MADV_SEQUENTIAL);
#endif

    return 0;
}

void mmap_close(mapped_file_t *mf) {
    if (mf->data && mf->size > 0)
        munmap(mf->data, mf->size);
    if (mf->fd >= 0)
        close(mf->fd);
    mf->data = NULL;
    mf->fd = -1;
    mf->size = 0;
}
