#include "FS.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

extern "C" {
void subStorageInit(void);

int arduino_fs_remove(const char *pathname);
int arduino_fs_rename(const char *oldpath, const char *newpath);
int arduino_fs_mkdir(const char *pathname, mode_t mode);
int arduino_fs_stat(const char *pathname, struct stat *buf);

typedef void DIR;
struct dirent {
    char d_name[128];
    unsigned char d_type;
};

FILE *file_fopen(const char *pathname, const char *mode);
int file_fclose(FILE *stream);
size_t file_fread(void *ptr, size_t size, size_t nitems, FILE *stream);
size_t file_fwrite(const void *ptr, size_t size, size_t nitems, FILE *stream);
int file_fseek(FILE *stream, long offset, int whence);
long file_ftell(FILE *stream);
int file_fstat(int fildes, struct stat *buf);

DIR *opendir(const char *pathname);
int closedir(DIR *dir);
struct dirent *readdir(DIR *dir);
}

namespace fs {
static const size_t kPathMaxLen = 192U;
static const size_t kNameMaxLen = 128U;
static const uint8_t kFlashPartitionC = 0U;

enum EntryType {
    ENTRY_TYPE_NONE = 0,
    ENTRY_TYPE_FILE,
    ENTRY_TYPE_DIRECTORY
};

struct StorageDirReadInfo {
    uint8_t type;
    uint32_t size;
    char name[256];
};

struct FileHandle {
    uint32_t refCount;
    EntryType type;
    bool writable;
    FILE *file;
    DIR *dir;
    long dirPosition;
    char mountPoint[4];
    char userPath[kPathMaxLen];
    char name[kNameMaxLen];
};

namespace {

static void copyString(char *dest, size_t destSize, const char *src)
{
    if (dest == NULL || destSize == 0U) {
        return;
    }

    if (src == NULL) {
        dest[0] = '\0';
        return;
    }

    snprintf(dest, destSize, "%s", src);
}

static bool isRootPath(const char *path)
{
    return path != NULL && strcmp(path, "/") == 0;
}

static bool normalizeUserPath(const char *path, char *normalized, size_t normalizedSize)
{
    const char *source = path;

    if (normalized == NULL || normalizedSize < 2U) {
        return false;
    }

    if (source == NULL || source[0] == '\0') {
        source = "/";
    }

    if (source[0] == '/') {
        if (snprintf(normalized, normalizedSize, "%s", source) >= (int)normalizedSize) {
            return false;
        }
    } else {
        if (snprintf(normalized, normalizedSize, "/%s", source) >= (int)normalizedSize) {
            return false;
        }
    }

    if (normalized[0] == '\0') {
        copyString(normalized, normalizedSize, "/");
    }

    return true;
}

static const char *basenameFromUserPath(const char *path)
{
    const char *lastSlash;

    if (path == NULL || path[0] == '\0') {
        return "";
    }

    if (strcmp(path, "/") == 0) {
        return "/";
    }

    lastSlash = strrchr(path, '/');
    if (lastSlash == NULL) {
        return path;
    }

    return lastSlash + 1;
}

static bool buildInternalPath(const char *mountPoint,
                              const char *userPath,
                              char *internalPath,
                              size_t internalPathSize)
{
    if (mountPoint == NULL || userPath == NULL || internalPath == NULL || internalPathSize < 4U) {
        return false;
    }

    if (isRootPath(userPath)) {
        return snprintf(internalPath, internalPathSize, "%s/", mountPoint) < (int)internalPathSize;
    }

    return snprintf(internalPath, internalPathSize, "%s%s", mountPoint, userPath) < (int)internalPathSize;
}

static bool buildChildUserPath(const char *parentPath,
                               const char *childName,
                               char *childPath,
                               size_t childPathSize)
{
    if (parentPath == NULL || childName == NULL || childPath == NULL || childPathSize == 0U) {
        return false;
    }

    if (strcmp(childName, ".") == 0 || strcmp(childName, "..") == 0) {
        return false;
    }

    if (isRootPath(parentPath)) {
        return snprintf(childPath, childPathSize, "/%s", childName) < (int)childPathSize;
    }

    return snprintf(childPath, childPathSize, "%s/%s", parentPath, childName) < (int)childPathSize;
}

static const char *dirEntryName(const struct dirent *entry)
{
    const StorageDirReadInfo *info = reinterpret_cast<const StorageDirReadInfo *>(entry);

    if (info == NULL) {
        return NULL;
    }

    return info->name;
}

static bool isWritableMode(const char *mode)
{
    if (mode == NULL) {
        return false;
    }

    return strchr(mode, 'w') != NULL || strchr(mode, 'a') != NULL || strchr(mode, '+') != NULL;
}

static void closeHandleResources(FileHandle *handle)
{
    if (handle == NULL) {
        return;
    }

    if (handle->file != NULL) {
        (void)file_fclose(handle->file);
        handle->file = NULL;
    }

    if (handle->dir != NULL) {
        (void)closedir(handle->dir);
        handle->dir = NULL;
    }

    handle->type = ENTRY_TYPE_NONE;
}

static FileHandle *allocateHandle(void)
{
    FileHandle *handle = new FileHandle();

    if (handle == NULL) {
        return NULL;
    }

    memset(handle, 0, sizeof(*handle));
    handle->refCount = 1U;
    return handle;
}

static bool statInternalPath(const char *internalPath, struct stat *st)
{
    if (internalPath == NULL || st == NULL) {
        return false;
    }

    memset(st, 0, sizeof(*st));
    return arduino_fs_stat(internalPath, st) == 0;
}

static FileHandle *openDirectoryHandle(const char *mountPoint, const char *userPath)
{
    char internalPath[kPathMaxLen] = {0};
    FileHandle *handle = NULL;

    if (!buildInternalPath(mountPoint, userPath, internalPath, sizeof(internalPath))) {
        return NULL;
    }

    handle = allocateHandle();
    if (handle == NULL) {
        return NULL;
    }

    handle->dir = opendir(internalPath);
    if (handle->dir == NULL) {
        delete handle;
        return NULL;
    }

    handle->type = ENTRY_TYPE_DIRECTORY;
    copyString(handle->mountPoint, sizeof(handle->mountPoint), mountPoint);
    copyString(handle->userPath, sizeof(handle->userPath), userPath);
    copyString(handle->name, sizeof(handle->name), basenameFromUserPath(userPath));
    return handle;
}

static FileHandle *openFileHandle(const char *mountPoint,
                                  const char *userPath,
                                  const char *mode,
                                  bool writable)
{
    char internalPath[kPathMaxLen] = {0};
    FileHandle *handle = NULL;

    if (!buildInternalPath(mountPoint, userPath, internalPath, sizeof(internalPath))) {
        return NULL;
    }

    handle = allocateHandle();
    if (handle == NULL) {
        return NULL;
    }

    handle->file = file_fopen(internalPath, mode);
    if (handle->file == NULL) {
        delete handle;
        return NULL;
    }

    handle->type = ENTRY_TYPE_FILE;
    handle->writable = writable;
    copyString(handle->mountPoint, sizeof(handle->mountPoint), mountPoint);
    copyString(handle->userPath, sizeof(handle->userPath), userPath);
    copyString(handle->name, sizeof(handle->name), basenameFromUserPath(userPath));
    return handle;
}

static FileHandle *openHandleFromUserPath(const char *mountPoint,
                                          const char *path,
                                          const char *mode,
                                          bool create)
{
    char userPath[kPathMaxLen] = {0};
    char internalPath[kPathMaxLen] = {0};
    struct stat st;
    bool pathExistsNow;
    bool writable = isWritableMode(mode);
    bool directoryPath = false;

    (void)create;

    if (mode == NULL) {
        mode = FILE_READ;
    }

    if (!normalizeUserPath(path, userPath, sizeof(userPath))) {
        return NULL;
    }

    if (isRootPath(userPath)) {
        return openDirectoryHandle(mountPoint, userPath);
    }

    if (!buildInternalPath(mountPoint, userPath, internalPath, sizeof(internalPath))) {
        return NULL;
    }

    pathExistsNow = statInternalPath(internalPath, &st);
    if (pathExistsNow && S_ISDIR(st.st_mode)) {
        directoryPath = true;
    } else {
        size_t pathLen = strlen(userPath);
        directoryPath = pathLen > 1U && userPath[pathLen - 1U] == '/';
    }

    if (directoryPath) {
        return openDirectoryHandle(mountPoint, userPath);
    }

    return openFileHandle(mountPoint, userPath, mode, writable);
}

static bool reopenDirectory(FileHandle *handle)
{
    char internalPath[kPathMaxLen] = {0};
    DIR *dir = NULL;

    if (handle == NULL || handle->type != ENTRY_TYPE_DIRECTORY) {
        return false;
    }

    if (!buildInternalPath(handle->mountPoint, handle->userPath, internalPath, sizeof(internalPath))) {
        return false;
    }

    dir = opendir(internalPath);
    if (dir == NULL) {
        return false;
    }

    if (handle->dir != NULL) {
        (void)closedir(handle->dir);
    }

    handle->dir = dir;
    handle->dirPosition = 0L;
    return true;
}

static File invalidFile(void)
{
    return File();
}

}  // namespace

File::File()
    : handle_(NULL)
{
}

File::File(FileHandle *handle)
    : handle_(handle)
{
}

File::File(const File &other)
    : Stream()
    , handle_(other.handle_)
{
    retain();
}

File &File::operator=(const File &other)
{
    if (this != &other) {
        release();
        handle_ = other.handle_;
        retain();
    }
    return *this;
}

File::~File()
{
    release();
}

void File::retain(void)
{
    if (handle_ != NULL) {
        handle_->refCount++;
    }
}

void File::release(void)
{
    if (handle_ == NULL) {
        return;
    }

    if (handle_->refCount > 1U) {
        handle_->refCount--;
        handle_ = NULL;
        return;
    }

    closeHandleResources(handle_);
    delete handle_;
    handle_ = NULL;
}

size_t File::write(uint8_t value)
{
    return write(&value, 1U);
}

size_t File::write(const uint8_t *buffer, size_t size)
{
    if (handle_ == NULL || handle_->type != ENTRY_TYPE_FILE || handle_->file == NULL || !handle_->writable) {
        return 0U;
    }

    return file_fwrite(buffer, 1U, size, handle_->file);
}

int File::available(void)
{
    size_t currentPosition;
    size_t totalSize;

    if (handle_ == NULL || handle_->type != ENTRY_TYPE_FILE) {
        return 0;
    }

    currentPosition = position();
    totalSize = size();
    if (totalSize < currentPosition) {
        return 0;
    }

    return (int)(totalSize - currentPosition);
}

int File::read(void)
{
    uint8_t value = 0U;

    if (read(&value, 1U) != 1U) {
        return -1;
    }

    return (int)value;
}

size_t File::read(uint8_t *buffer, size_t size)
{
    if (handle_ == NULL || handle_->type != ENTRY_TYPE_FILE || handle_->file == NULL || buffer == NULL) {
        return 0U;
    }

    return file_fread(buffer, 1U, size, handle_->file);
}

int File::peek(void)
{
    size_t currentPosition;
    uint8_t value = 0U;

    if (handle_ == NULL || handle_->type != ENTRY_TYPE_FILE || handle_->file == NULL) {
        return -1;
    }

    currentPosition = position();
    if (read(&value, 1U) != 1U) {
        return -1;
    }

    (void)seek((uint32_t)currentPosition, SeekSet);
    return (int)value;
}

void File::flush(void)
{
    if (handle_ == NULL || handle_->type != ENTRY_TYPE_FILE || handle_->file == NULL) {
        return;
    }

    /* SDK storage wrapper flushes writes immediately for the internal LFS path. */
}

bool File::seek(uint32_t pos, SeekMode mode)
{
    int whence = SEEK_SET;

    if (handle_ == NULL || handle_->type != ENTRY_TYPE_FILE || handle_->file == NULL) {
        return false;
    }

    switch (mode) {
        case SeekSet:
            whence = SEEK_SET;
            break;
        case SeekCur:
            whence = SEEK_CUR;
            break;
        case SeekEnd:
            whence = SEEK_END;
            break;
        default:
            return false;
    }

    return file_fseek(handle_->file, (long)pos, whence) == 0;
}

size_t File::position(void) const
{
    long offset;

    if (handle_ == NULL || handle_->type != ENTRY_TYPE_FILE || handle_->file == NULL) {
        return (size_t)-1;
    }

    offset = file_ftell(handle_->file);
    if (offset < 0) {
        return (size_t)-1;
    }

    return (size_t)offset;
}

size_t File::size(void) const
{
    struct stat st;

    if (handle_ == NULL) {
        return 0U;
    }

    if (handle_->type == ENTRY_TYPE_DIRECTORY) {
        return 0U;
    }

    memset(&st, 0, sizeof(st));
    if (handle_->file == NULL || file_fstat((int)handle_->file, &st) != 0) {
        return 0U;
    }

    return (size_t)st.st_size;
}

bool File::setBufferSize(size_t size)
{
    (void)size;
    return false;
}

void File::close(void)
{
    if (handle_ == NULL) {
        return;
    }

    closeHandleResources(handle_);
}

File::operator bool() const
{
    return handle_ != NULL && handle_->type != ENTRY_TYPE_NONE;
}

time_t File::getLastWrite(void) const
{
    return (time_t)0;
}

const char *File::path(void) const
{
    return handle_ != NULL ? handle_->userPath : NULL;
}

const char *File::name(void) const
{
    return handle_ != NULL ? handle_->name : NULL;
}

bool File::isDirectory(void) const
{
    return handle_ != NULL && handle_->type == ENTRY_TYPE_DIRECTORY;
}

bool File::seekDir(long position)
{
    long index;

    if (handle_ == NULL || handle_->type != ENTRY_TYPE_DIRECTORY || position < 0) {
        return false;
    }

    if (!reopenDirectory(handle_)) {
        return false;
    }

    for (index = 0L; index < position; index++) {
        if (!openNextFile()) {
            return false;
        }
    }

    handle_->dirPosition = position;
    return true;
}

File File::openNextFile(const char *mode)
{
    struct dirent *entry = NULL;
    char childPath[kPathMaxLen] = {0};
    FileHandle *child = NULL;
    const char *childName = NULL;

    if (handle_ == NULL || handle_->type != ENTRY_TYPE_DIRECTORY || handle_->dir == NULL) {
        return invalidFile();
    }

    while ((entry = readdir(handle_->dir)) != NULL) {
        childName = dirEntryName(entry);
        if (childName == NULL || childName[0] == '\0') {
            continue;
        }

        if (!buildChildUserPath(handle_->userPath, childName, childPath, sizeof(childPath))) {
            continue;
        }

        child = openHandleFromUserPath(handle_->mountPoint, childPath, mode, false);
        if (child != NULL) {
            handle_->dirPosition++;
            return File(child);
        }
    }

    return invalidFile();
}

String File::getNextFileName(void)
{
    bool isDir = false;
    return getNextFileName(&isDir);
}

String File::getNextFileName(bool *isDir)
{
    File entry = openNextFile();

    if (!entry) {
        if (isDir != NULL) {
            *isDir = false;
        }
        return String("");
    }

    if (isDir != NULL) {
        *isDir = entry.isDirectory();
    }

    return String(entry.name());
}

void File::rewindDirectory(void)
{
    if (handle_ == NULL || handle_->type != ENTRY_TYPE_DIRECTORY) {
        return;
    }

    (void)reopenDirectory(handle_);
}

FS::FS(const char *mountPoint)
    : ready_(true)
{
    copyString(mountPoint_, sizeof(mountPoint_), mountPoint != NULL ? mountPoint : "C:");
}

File FS::open(const char *path, const char *mode, const bool create)
{
    if (!ready_) {
        return invalidFile();
    }

    return File(openHandleFromUserPath(mountPoint_, path, mode, create));
}

File FS::open(const String &path, const char *mode, const bool create)
{
    return open(path.c_str(), mode, create);
}

bool FS::exists(const char *path)
{
    char userPath[kPathMaxLen] = {0};
    char internalPath[kPathMaxLen] = {0};
    struct stat st;

    if (!ready_) {
        return false;
    }

    if (!normalizeUserPath(path, userPath, sizeof(userPath))) {
        return false;
    }

    if (isRootPath(userPath)) {
        return true;
    }

    if (!buildInternalPath(mountPoint_, userPath, internalPath, sizeof(internalPath))) {
        return false;
    }

    return statInternalPath(internalPath, &st);
}

bool FS::exists(const String &path)
{
    return exists(path.c_str());
}

bool FS::remove(const char *path)
{
    char userPath[kPathMaxLen] = {0};
    char internalPath[kPathMaxLen] = {0};

    if (!ready_) {
        return false;
    }

    if (!normalizeUserPath(path, userPath, sizeof(userPath)) || isRootPath(userPath)) {
        return false;
    }

    if (!buildInternalPath(mountPoint_, userPath, internalPath, sizeof(internalPath))) {
        return false;
    }

    return arduino_fs_remove(internalPath) == 0;
}

bool FS::remove(const String &path)
{
    return remove(path.c_str());
}

bool FS::rename(const char *pathFrom, const char *pathTo)
{
    char fromPath[kPathMaxLen] = {0};
    char toPath[kPathMaxLen] = {0};
    char internalFrom[kPathMaxLen] = {0};
    char internalTo[kPathMaxLen] = {0};

    if (!ready_) {
        return false;
    }

    if (!normalizeUserPath(pathFrom, fromPath, sizeof(fromPath)) ||
        !normalizeUserPath(pathTo, toPath, sizeof(toPath)) ||
        isRootPath(fromPath) ||
        isRootPath(toPath)) {
        return false;
    }

    if (!buildInternalPath(mountPoint_, fromPath, internalFrom, sizeof(internalFrom)) ||
        !buildInternalPath(mountPoint_, toPath, internalTo, sizeof(internalTo))) {
        return false;
    }

    return arduino_fs_rename(internalFrom, internalTo) == 0;
}

bool FS::rename(const String &pathFrom, const String &pathTo)
{
    return rename(pathFrom.c_str(), pathTo.c_str());
}

bool FS::mkdir(const char *path)
{
    char userPath[kPathMaxLen] = {0};
    char internalPath[kPathMaxLen] = {0};

    if (!ready_) {
        return false;
    }

    if (!normalizeUserPath(path, userPath, sizeof(userPath))) {
        return false;
    }

    if (isRootPath(userPath)) {
        return true;
    }

    if (!buildInternalPath(mountPoint_, userPath, internalPath, sizeof(internalPath))) {
        return false;
    }

    return arduino_fs_mkdir(internalPath, 0) == 0;
}

bool FS::mkdir(const String &path)
{
    return mkdir(path.c_str());
}

bool FS::rmdir(const char *path)
{
    char userPath[kPathMaxLen] = {0};
    char internalPath[kPathMaxLen] = {0};

    if (!ready_) {
        return false;
    }

    if (!normalizeUserPath(path, userPath, sizeof(userPath)) || isRootPath(userPath)) {
        return false;
    }

    if (!buildInternalPath(mountPoint_, userPath, internalPath, sizeof(internalPath))) {
        return false;
    }

    return arduino_fs_remove(internalPath) == 0;
}

bool FS::rmdir(const String &path)
{
    return rmdir(path.c_str());
}

const char *FS::mountpoint(void) const
{
    return mountPoint_;
}

}  // namespace fs
