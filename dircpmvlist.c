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
#include <libgen.h>
#include <dirent.h>

char *source_dir;
char *destination_dir;
char *extensions[6];
int extension_count = 0;
int is_move = 0;

void create_destination_directory(const char *dest_path) {
    if (mkdir(dest_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
        if (errno != EEXIST) {
            perror("mkdir");
            exit(EXIT_FAILURE);
        }
    }
}

int has_extension(const char *filename) {
    if (extension_count == 0)
        return 1;

    for (int i = 0; i < extension_count; i++) {
        if (strstr(filename, extensions[i])) {
            return 1;
        }
    }
    return 0;
}

int perform_file_operation(const char *src_path, const char *dest_path) {
    if (!has_extension(src_path)) {
        return 0;
    }

    if (is_move) {
        if (rename(src_path, dest_path) != 0) {
            perror("rename");
            return -1;
        }
    } else {
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
    }

    return 0;
}

int copy_item(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    char dest_path[PATH_MAX];

    snprintf(dest_path, PATH_MAX, "%s/%s", destination_dir, fpath + strlen(source_dir));

    if (typeflag == FTW_D || typeflag == FTW_DP) {
        create_destination_directory(dest_path);
    } else if (typeflag == FTW_F) {
        return perform_file_operation(fpath, dest_path);
    }

    // For directories, if it's a move operation, use rename to move the directory
    if (is_move && (typeflag == FTW_D || typeflag == FTW_DP)) {
        if (rename(fpath, dest_path) != 0) {
            return -1;
        }
    }

    return 0;
}


int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 11) {
        fprintf(stderr, "Usage: %s <src_path> <dest_path> [-cp/-mv <extension1> ... <extension6>]\n", argv[0]);
        return EXIT_FAILURE;
    }

    source_dir = argv[1];
    destination_dir = argv[2];

    struct stat src_stat;
    if (stat(source_dir, &src_stat) != 0 || !S_ISDIR(src_stat.st_mode)) {
        fprintf(stderr, "Source directory does not exist: %s\n", source_dir);
        return EXIT_FAILURE;
    }

    if (argc > 3) {
        if (strcmp(argv[3], "-cp") == 0) {
            is_move = 0;
        } else if (strcmp(argv[3], "-mv") == 0) {
            is_move = 1;
        } else {
            fprintf(stderr, "Invalid flag: %s. Please provide -cp or -mv flag.\n", argv[3]);
            fprintf(stderr, "Usage: %s <src_path> <dest_path> [-cp/-mv <extension1> ... <extension6>]\n", argv[0]);
            return EXIT_FAILURE;
        }
    } else {
        fprintf(stderr, "No flag provided. Please provide -cp or -mv flag.\n");
        fprintf(stderr, "Usage: %s <src_path> <dest_path> [-cp/-mv <extension1> ... <extension6>]\n", argv[0]);
        return EXIT_FAILURE;
    }

    for (int i = 4; i < argc && i < 10; i++) {
        extensions[i - 4] = argv[i];
        extension_count++;
    }

    create_destination_directory(destination_dir);

    int flags = FTW_PHYS;
    int result = nftw(source_dir, copy_item, 20, flags);

    if (extension_count > 0) {
        if (is_move) {
            printf("Files with specified extensions moved from %s to %s.\n", source_dir, destination_dir);
        } else {
            printf("Files with specified extensions copied from %s to %s.\n", source_dir, destination_dir);
        }
    } else {
        if (is_move) {
            printf("All files and directories moved from %s to %s.\n", source_dir, destination_dir);
        } else {
            printf("All files and directories copied from %s to %s.\n", source_dir, destination_dir);
        }
    }

    return (result == -1) ? EXIT_FAILURE : EXIT_SUCCESS;
}

