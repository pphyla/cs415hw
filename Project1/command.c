#include "command.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void write_all(const char *text, size_t length)
{
    size_t written = 0;

    while (written < length) {
        ssize_t result = write(STDOUT_FILENO, text + written, length - written);
        if (result <= 0) {
            return;
        }
        written += (size_t)result;
    }
}

static void write_text(const char *text)
{
    write_all(text, strlen(text));
}

static int compare_names(const void *left, const void *right)
{
    const char *const *left_name = left;
    const char *const *right_name = right;
    return strcmp(*left_name, *right_name);
}

void listDir(void)
{
    DIR *directory = opendir(".");
    struct dirent *entry;
    char **names = NULL;
    size_t count = 0;
    size_t index;

    if (directory == NULL) {
        return;
    }

    while ((entry = readdir(directory)) != NULL) {
        char **new_names = realloc(names, sizeof(*names) * (count + 1));
        if (new_names == NULL) {
            for (index = 0; index < count; index++) {
                free(names[index]);
            }
            free(names);
            closedir(directory);
            return;
        }
        names = new_names;
        names[count] = malloc(strlen(entry->d_name) + 1);
        if (names[count] == NULL) {
            for (index = 0; index < count; index++) {
                free(names[index]);
            }
            free(names);
            closedir(directory);
            return;
        }
        strcpy(names[count], entry->d_name);
        count++;
    }
    closedir(directory);

    qsort(names, count, sizeof(*names), compare_names);
    for (index = 0; index < count; index++) {
        write_text(names[index]);
        write_text(" ");
        free(names[index]);
    }
    write_text("\n");
    free(names);
}

void showCurrentDir(void)
{
    char *path = getcwd(NULL, 0);
    if (path == NULL) {
        return;
    }
    write_text(path);
    write_text("\n");
    free(path);
}

void makeDir(char *dirName)
{
    if (mkdir(dirName, 0755) == -1 && errno == EEXIST) {
        write_text("Directory already exists!\n");
    }
}

void changeDir(char *dirName)
{
    chdir(dirName);
}

static char *destination_path(const char *source, const char *destination)
{
    struct stat destination_status;
    const char *last_slash;
    const char *source_name;
    size_t destination_length;
    size_t source_length;
    char *path;

    if (stat(destination, &destination_status) == -1 ||
        !S_ISDIR(destination_status.st_mode)) {
        path = malloc(strlen(destination) + 1);
        if (path != NULL) {
            strcpy(path, destination);
        }
        return path;
    }

    last_slash = strrchr(source, '/');
    source_name = last_slash == NULL ? source : last_slash + 1;
    destination_length = strlen(destination);
    source_length = strlen(source_name);
    path = malloc(destination_length + source_length + 2);
    if (path == NULL) {
        return NULL;
    }
    strcpy(path, destination);
    if (destination_length > 0 && destination[destination_length - 1] != '/') {
        path[destination_length] = '/';
        destination_length++;
    }
    strcpy(path + destination_length, source_name);
    return path;
}

static int copy_contents(const char *source, const char *destination)
{
    int source_fd = open(source, O_RDONLY);
    int destination_fd;
    char buffer[4096];
    ssize_t bytes_read;

    if (source_fd == -1) {
        return -1;
    }
    destination_fd = open(destination, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (destination_fd == -1) {
        close(source_fd);
        return -1;
    }

    while ((bytes_read = read(source_fd, buffer, sizeof(buffer))) > 0) {
        ssize_t offset = 0;
        while (offset < bytes_read) {
            ssize_t bytes_written = write(destination_fd, buffer + offset,
                                          (size_t)(bytes_read - offset));
            if (bytes_written <= 0) {
                close(source_fd);
                close(destination_fd);
                return -1;
            }
            offset += bytes_written;
        }
    }
    close(source_fd);
    close(destination_fd);
    return bytes_read == 0 ? 0 : -1;
}

void copyFile(char *sourcePath, char *destinationPath)
{
    struct stat source_status;
    char *actual_destination;

    if (stat(sourcePath, &source_status) == -1 ||
        !S_ISREG(source_status.st_mode)) {
        return;
    }
    actual_destination = destination_path(sourcePath, destinationPath);
    if (actual_destination == NULL) {
        return;
    }
    copy_contents(sourcePath, actual_destination);
    free(actual_destination);
}

void moveFile(char *sourcePath, char *destinationPath)
{
    char *actual_destination = destination_path(sourcePath, destinationPath);

    if (actual_destination == NULL) {
        return;
    }
    if (copy_contents(sourcePath, actual_destination) == 0) {
        unlink(sourcePath);
    }
    free(actual_destination);
}

void deleteFile(char *filename)
{
    struct stat file_status;
    if (stat(filename, &file_status) == 0 && S_ISREG(file_status.st_mode)) {
        unlink(filename);
    }
}

void displayFile(char *filename)
{
    int file = open(filename, O_RDONLY);
    char buffer[4096];
    ssize_t bytes_read;

    if (file == -1) {
        return;
    }
    while ((bytes_read = read(file, buffer, sizeof(buffer))) > 0) {
        ssize_t offset = 0;
        while (offset < bytes_read) {
            ssize_t bytes_written = write(STDOUT_FILENO, buffer + offset,
                                          (size_t)(bytes_read - offset));
            if (bytes_written <= 0) {
                close(file);
                return;
            }
            offset += bytes_written;
        }
    }
    close(file);
}
