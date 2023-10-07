#define _XOPEN_SOURCE 500
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ftw.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>

char *dest_dir;  // Define dest_dir as a global variable

void create_destination_directory(const char *dest_path) {
    if (mkdir(dest_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
        if (errno != EEXIST) {
            perror("mkdir");
            exit(EXIT_FAILURE);
        }
    }
}

int copy_file(const char *src_path, const char *dest_path) {
      FILE *src_file = fopen(src_path, "rb");
    if (src_file == NULL) {
        perror("fopen(src)");
        return -1;
    }

    FILE *dest_file = fopen(dest_path, "wb");
    if (dest_file == NULL) {
        perror("fopen(dest)");
        fclose(src_file);
        return -1;
    }

    char buffer[BUFSIZ];
    size_t bytesRead;

    while ((bytesRead = fread(buffer, 1, BUFSIZ, src_file)) > 0) {
        size_t bytesWritten = fwrite(buffer, 1, bytesRead, dest_file);
        if (bytesWritten != bytesRead) {
            perror("fwrite");
            fclose(src_file);
            fclose(dest_file);
            return -1;
        }
    }

    fclose(src_file);
    fclose(dest_file);
    return 0;
}

static int ls_copy_callback(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    if (typeflag == FTW_F) {
        // Build the destination path
        char dest_path[PATH_MAX];
        snprintf(dest_path, PATH_MAX, "%s/%s", dest_dir, fpath + ftwbuf->base);

        // Create destination directory if it doesn't exist
        create_destination_directory(dest_dir);

        // Copy the file
        if (copy_file(fpath, dest_path) == -1) {
            fprintf(stderr, "Failed to copy file from %s to %s\n", fpath, dest_path);
        }
    }

    return 0;
}


int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <src_path> <dest_path> <extension>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *src_dir = argv[1];
    dest_dir = argv[2];  // Assign to global variable
    const char *extension = argv[3];

    create_destination_directory(dest_dir);

    if (nftw(src_dir, ls_copy_callback, 10, FTW_PHYS) == -1) {
        perror("nftw");
        return EXIT_FAILURE;
    }

    printf("Files with extension '%s' copied from %s to %s.\n", extension, src_dir, dest_dir);

    return EXIT_SUCCESS;
}
