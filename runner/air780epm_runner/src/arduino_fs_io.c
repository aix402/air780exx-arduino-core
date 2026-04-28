#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "lfs_port.h"

#define ARDUINO_FS_PATH_MAX 192U

typedef struct ArduinoDirEntry
{
    uint8_t type;
    uint32_t size;
    char name[256];
} ArduinoDirEntry;

typedef struct ArduinoDirHandle
{
    lfs_dir_t dir;
    ArduinoDirEntry entry;
} DIR;

struct dirent
{
    uint8_t type;
    uint32_t size;
    char name[256];
};

static int arduinoFsEnsureReady(void)
{
    static int initialized = 0;

    if (initialized != 0)
    {
        return 0;
    }

    if (LFS_init() != 0)
    {
        return -1;
    }

    initialized = 1;
    return 0;
}

static int arduinoFsNormalizePath(const char *input, char *output, size_t outputSize)
{
    const char *source = input;
    size_t length;

    if ((output == NULL) || (outputSize < 2U))
    {
        return -1;
    }

    if ((source == NULL) || (source[0] == '\0'))
    {
        source = "/";
    }

    if ((source[0] != '\0') && (source[1] == ':'))
    {
        source += 2;
    }

    if (source[0] == '\0')
    {
        source = "/";
    }

    if (source[0] != '/')
    {
        if (snprintf(output, outputSize, "/%s", source) >= (int)outputSize)
        {
            return -1;
        }
    }
    else if (snprintf(output, outputSize, "%s", source) >= (int)outputSize)
    {
        return -1;
    }

    length = strlen(output);
    while ((length > 1U) && (output[length - 1U] == '/'))
    {
        output[length - 1U] = '\0';
        length--;
    }

    return 0;
}

static int arduinoFsModeToFlags(const char *mode)
{
    if ((mode == NULL) || (!strcmp(mode, "r")) || (!strcmp(mode, "rb")))
    {
        return LFS_O_RDONLY;
    }

    if ((!strcmp(mode, "r+")) || (!strcmp(mode, "r+b")) || (!strcmp(mode, "rb+")))
    {
        return LFS_O_RDWR | LFS_O_CREAT;
    }

    if ((!strcmp(mode, "w")) || (!strcmp(mode, "wb")))
    {
        return LFS_O_RDWR | LFS_O_CREAT | LFS_O_TRUNC;
    }

    if ((!strcmp(mode, "w+")) || (!strcmp(mode, "w+b")) || (!strcmp(mode, "wb+")))
    {
        return LFS_O_RDWR | LFS_O_CREAT | LFS_O_TRUNC;
    }

    if ((!strcmp(mode, "a")) || (!strcmp(mode, "ab")))
    {
        return LFS_O_WRONLY | LFS_O_APPEND | LFS_O_CREAT;
    }

    if ((!strcmp(mode, "a+")) || (!strcmp(mode, "a+b")) || (!strcmp(mode, "ab+")))
    {
        return LFS_O_RDWR | LFS_O_APPEND | LFS_O_CREAT;
    }

    return LFS_O_RDONLY;
}

int arduino_fs_stat(const char *pathname, struct stat *buf)
{
    char path[ARDUINO_FS_PATH_MAX] = {0};
    struct lfs_info info = {0};

    if ((buf == NULL) || (arduinoFsEnsureReady() != 0) || (arduinoFsNormalizePath(pathname, path, sizeof(path)) != 0))
    {
        return -1;
    }

    memset(buf, 0, sizeof(*buf));
    if (!strcmp(path, "/"))
    {
        buf->st_mode = S_IFDIR;
        buf->st_size = 0;
        return 0;
    }

    if (LFS_stat(path, &info) != 0)
    {
        return -1;
    }

    buf->st_mode = (info.type == LFS_TYPE_DIR) ? S_IFDIR : S_IFREG;
    buf->st_size = (off_t)info.size;
    return 0;
}

int arduino_fs_remove(const char *pathname)
{
    char path[ARDUINO_FS_PATH_MAX] = {0};

    if ((arduinoFsEnsureReady() != 0) || (arduinoFsNormalizePath(pathname, path, sizeof(path)) != 0) || (!strcmp(path, "/")))
    {
        return -1;
    }

    return LFS_remove(path);
}

int arduino_fs_rename(const char *oldpath, const char *newpath)
{
    char fromPath[ARDUINO_FS_PATH_MAX] = {0};
    char toPath[ARDUINO_FS_PATH_MAX] = {0};

    if ((arduinoFsEnsureReady() != 0) ||
        (arduinoFsNormalizePath(oldpath, fromPath, sizeof(fromPath)) != 0) ||
        (arduinoFsNormalizePath(newpath, toPath, sizeof(toPath)) != 0) ||
        (!strcmp(fromPath, "/")) ||
        (!strcmp(toPath, "/")))
    {
        return -1;
    }

    return LFS_rename(fromPath, toPath);
}

int arduino_fs_mkdir(const char *pathname, mode_t mode)
{
    char path[ARDUINO_FS_PATH_MAX] = {0};

    (void)mode;

    if ((arduinoFsEnsureReady() != 0) || (arduinoFsNormalizePath(pathname, path, sizeof(path)) != 0))
    {
        return -1;
    }

    if (!strcmp(path, "/"))
    {
        return 0;
    }

    return LFS_mkdir(path);
}

FILE *file_fopen(const char *pathname, const char *mode)
{
    char path[ARDUINO_FS_PATH_MAX] = {0};
    lfs_file_t *file;
    const int flags = arduinoFsModeToFlags(mode);

    if ((arduinoFsEnsureReady() != 0) || (arduinoFsNormalizePath(pathname, path, sizeof(path)) != 0) || (!strcmp(path, "/")))
    {
        return NULL;
    }

    file = (lfs_file_t *)malloc(sizeof(lfs_file_t));
    if (file == NULL)
    {
        return NULL;
    }

    memset(file, 0, sizeof(*file));
    if (LFS_fileOpen(file, path, flags) != 0)
    {
        free(file);
        return NULL;
    }

    return (FILE *)file;
}

int file_fclose(FILE *stream)
{
    if (stream == NULL)
    {
        return 0;
    }

    (void)LFS_fileClose((lfs_file_t *)stream);
    free(stream);
    return 0;
}

size_t file_fread(void *ptr, size_t size, size_t nitems, FILE *stream)
{
    lfs_ssize_t bytesRead;

    if ((stream == NULL) || (ptr == NULL) || (size == 0U) || (nitems == 0U) || (arduinoFsEnsureReady() != 0))
    {
        return 0U;
    }

    bytesRead = LFS_fileRead((lfs_file_t *)stream, ptr, size * nitems);
    return (bytesRead < 0) ? 0U : (size_t)bytesRead;
}

size_t file_fwrite(const void *ptr, size_t size, size_t nitems, FILE *stream)
{
    lfs_ssize_t bytesWritten;

    if ((stream == NULL) || (ptr == NULL) || (size == 0U) || (nitems == 0U) || (arduinoFsEnsureReady() != 0))
    {
        return 0U;
    }

    bytesWritten = LFS_fileWrite((lfs_file_t *)stream, ptr, size * nitems);
    return (bytesWritten < 0) ? 0U : (size_t)bytesWritten;
}

int file_fseek(FILE *stream, long offset, int whence)
{
    if ((stream == NULL) || (arduinoFsEnsureReady() != 0))
    {
        return -1;
    }

    return (LFS_fileSeek((lfs_file_t *)stream, offset, whence) < 0) ? -1 : 0;
}

long file_ftell(FILE *stream)
{
    lfs_soff_t offset;

    if ((stream == NULL) || (arduinoFsEnsureReady() != 0))
    {
        return -1L;
    }

    offset = LFS_fileTell((lfs_file_t *)stream);
    return (offset < 0) ? -1L : (long)offset;
}

int file_fstat(int fildes, struct stat *buf)
{
    lfs_soff_t size;
    FILE *stream = (FILE *)(uintptr_t)fildes;

    if ((stream == NULL) || (buf == NULL) || (arduinoFsEnsureReady() != 0))
    {
        return -1;
    }

    memset(buf, 0, sizeof(*buf));
    size = LFS_fileSize((lfs_file_t *)stream);
    if (size < 0)
    {
        return -1;
    }

    buf->st_mode = S_IFREG;
    buf->st_size = (off_t)size;
    return 0;
}

DIR *opendir(const char *pathname)
{
    char path[ARDUINO_FS_PATH_MAX] = {0};
    DIR *dir;

    if ((arduinoFsEnsureReady() != 0) || (arduinoFsNormalizePath(pathname, path, sizeof(path)) != 0))
    {
        return NULL;
    }

    dir = (DIR *)malloc(sizeof(*dir));
    if (dir == NULL)
    {
        return NULL;
    }

    memset(dir, 0, sizeof(*dir));
    if (LFS_dirOpen(&dir->dir, path) != 0)
    {
        free(dir);
        return NULL;
    }

    return dir;
}

int closedir(DIR *dir)
{
    if (dir == NULL)
    {
        return 0;
    }

    (void)LFS_dirClose(&dir->dir);
    free(dir);
    return 0;
}

struct dirent *readdir(DIR *dir)
{
    struct lfs_info info = {0};
    int result;

    if ((dir == NULL) || (arduinoFsEnsureReady() != 0))
    {
        return NULL;
    }

    while (1)
    {
        result = LFS_dirRead(&dir->dir, &info);
        if (result <= 0)
        {
            return NULL;
        }

        if ((info.type == LFS_TYPE_DIR) &&
            ((!strcmp(info.name, ".")) || (!strcmp(info.name, ".."))))
        {
            continue;
        }

        memset(&dir->entry, 0, sizeof(dir->entry));
        dir->entry.type = (uint8_t)((info.type == LFS_TYPE_DIR) ? 1U : 0U);
        dir->entry.size = (uint32_t)info.size;
        snprintf(dir->entry.name, sizeof(dir->entry.name), "%s", info.name);
        return (struct dirent *)&dir->entry;
    }
}

int32_t getFsTotalSize(uint8_t partation)
{
    lfs_status_t status = {0};

    (void)partation;

    if ((arduinoFsEnsureReady() != 0) || (LFS_statfs(&status) != 0))
    {
        return -1;
    }

    return (int32_t)(status.total_block * status.block_size);
}

int32_t getFsFreeSize(uint8_t partation)
{
    lfs_status_t status = {0};

    (void)partation;

    if ((arduinoFsEnsureReady() != 0) || (LFS_statfs(&status) != 0) || (status.total_block < status.block_used))
    {
        return -1;
    }

    return (int32_t)((status.total_block - status.block_used) * status.block_size);
}
