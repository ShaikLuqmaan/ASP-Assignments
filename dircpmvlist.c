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

// Define dest_dir as a global variable
char *dest_dir;  
// Array to store provided extensions
char *extensions[6]; 
int extension_count = 0;


// Function to create destination directory if not present
void ls_create_destination_directory(const char *dest_path) {
    if (mkdir(dest_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
        if (errno != EEXIST) {
            perror("mkdir");
            exit(EXIT_FAILURE);
        }
    }
}

// Function to check if extension argument is provided by the user
int ls_has_extension(const char *filename) {
    if (extension_count == 0)
        return 1; // No extension list provided, copy all files

    for (int i = 0; i < extension_count; i++) {
        if (strstr(filename, extensions[i])) {
            return 1; // File has a matching extension, copy it
        }
    }
    return 0; // File does not have a matching extension, skip it
}

// Function to perform file operations
int ls_perform_file_operation(const char *src_path, const char *dest_path, int is_move) {
    // Check if the file has a valid extension
    if (!ls_has_extension(src_path)) {
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

// Recursive Function for traversal 
void ls_copy_contents(const char *src_path, const char *dest_path, int is_move) {
    struct stat st;
    if (stat(src_path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            DIR *dir = opendir(src_path);
            if (dir == NULL) {
                perror("opendir");
                return;
            }

            ls_create_destination_directory(dest_path);

            struct dirent *entry;
            while ((entry = readdir(dir))) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                    continue;

                char sub_src[PATH_MAX];
                char sub_dest[PATH_MAX];
                snprintf(sub_src, PATH_MAX, "%s/%s", src_path, entry->d_name);
                snprintf(sub_dest, PATH_MAX, "%s/%s", dest_path, entry->d_name);

                ls_copy_contents(sub_src, sub_dest, is_move);
            }

            closedir(dir);

            if (is_move) {
                if (remove(src_path) != 0) {
                    perror("remove");
                }
            }
        } else if (S_ISREG(st.st_mode)) {
            char dest_dir_copy[PATH_MAX];
            strcpy(dest_dir_copy, dest_path);
            ls_create_destination_directory(dirname(dest_dir_copy));

            // Copy or move the file
            ls_perform_file_operation(src_path, dest_path, is_move);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 11) {
        fprintf(stderr, "Usage: %s <src_path> <dest_path> [-cp/-mv <extension1> ... <extension6>]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *src_dir = argv[1];
    dest_dir = argv[2];  

    // Check if the source directory exists
    struct stat src_stat;
    if (stat(src_dir, &src_stat) != 0 || !S_ISDIR(src_stat.st_mode)) {
        fprintf(stderr, "Source directory does not exist: %s\n", src_dir);
        return EXIT_FAILURE;
    }

    // Flag to indicate whether to move or copy
    int is_move = 0;  

    // Check if extensions are provided
    if (argc > 3) {
        if (strcmp(argv[3], "-cp") == 0) {
            // Extensions are provided for copying
            is_move = 0;
        } else if (strcmp(argv[3], "-mv") == 0) {
            // Extensions are provided for moving
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

    // Store provided extensions
    for (int i = 4; i < argc && i < 10; i++) {
        extensions[i - 4] = argv[i];
        extension_count++;
    }

    ls_create_destination_directory(dest_dir);

    // Copy or move the contents
    ls_copy_contents(src_dir, dest_dir, is_move);

    if (extension_count > 0) {
        if (is_move) {
            printf("Files with specified extensions moved from %s to %s.\n", src_dir, dest_dir);
        } else {
            printf("Files with specified extensions copied from %s to %s.\n", src_dir, dest_dir);
        }
    } else {
        if (is_move) {
            printf("All files and directories moved from %s to %s.\n", src_dir, dest_dir);
        } else {
            printf("All files and directories copied from %s to %s.\n", src_dir, dest_dir);
        }
    }

    return EXIT_SUCCESS;
}
