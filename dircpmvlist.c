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

char *dest_dir;  // Define dest_dir as a global variable
char *extensions[6]; // Array to store extensions
int extension_count = 0;

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
        return 1; // No extension list provided, copy all files

    for (int i = 0; i < extension_count; i++) {
        if (strstr(filename, extensions[i])) {
            return 1; // File has a matching extension, copy it
        }
    }
    return 0; // File does not have a matching extension, skip it
}

int copy_file(const char *src_path, const char *dest_path) {
    // Check if the file has a valid extension
    if (!has_extension(src_path)) {
        return 0;
    }

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

void copy_contents(const char *src_path, const char *dest_path) {
    struct stat st;
    if (stat(src_path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            // It's a directory, so recursively copy its contents
            DIR *dir = opendir(src_path);
            if (dir == NULL) {
                perror("opendir");
                return;
            }

            // Create destination directory if it doesn't exist
            create_destination_directory(dest_path);

            struct dirent *entry;
            while ((entry = readdir(dir))) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                    continue;

                char sub_src[PATH_MAX];
                char sub_dest[PATH_MAX];
                snprintf(sub_src, PATH_MAX, "%s/%s", src_path, entry->d_name);
                snprintf(sub_dest, PATH_MAX, "%s/%s", dest_path, entry->d_name);

                copy_contents(sub_src, sub_dest);
            }

            closedir(dir);
        } else if (S_ISREG(st.st_mode)) {
            // It's a regular file, so copy it
            // Create destination directory if it doesn't exist
            char dest_dir_copy[PATH_MAX];
            strcpy(dest_dir_copy, dest_path);
            create_destination_directory(dirname(dest_dir_copy));

            // Copy the file
            copy_file(src_path, dest_path);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 11) {
        fprintf(stderr, "Usage: %s <src_path> <dest_path> [-cp <extension1> ... <extension6>]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *src_dir = argv[1];
    dest_dir = argv[2];  // Assign to global variable

    // Check if extensions are provided
    if (argc > 3 && strcmp(argv[3], "-cp") == 0) {
        // Extensions are provided, store them
        for (int i = 4; i < argc && i < 10; i++) {
            extensions[i - 4] = argv[i];
            extension_count++;
        }
    } else {
        // No extensions provided, copy all files
        extension_count = 0;
    }

    create_destination_directory(dest_dir);

    // Copy the contents
    copy_contents(src_dir, dest_dir);

    if (extension_count > 0) {
        printf("Files with specified extensions copied from %s to %s.\n", src_dir, dest_dir);
    } else {
        printf("All files and directories copied from %s to %s.\n", src_dir, dest_dir);
    }

    return EXIT_SUCCESS;
}
