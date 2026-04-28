#ifndef FS_H
#define FS_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "Stream.h"
#include "WString.h"

namespace fs {

#define FILE_READ   "r"
#define FILE_WRITE  "w"
#define FILE_APPEND "a"

struct FileHandle;

enum SeekMode {
    SeekSet = 0,
    SeekCur = 1,
    SeekEnd = 2
};

class File : public Stream {
public:
    File();
    explicit File(FileHandle *handle);
    File(const File &other);
    File &operator=(const File &other);
    virtual ~File();

    using Print::write;

    size_t write(uint8_t value) override;
    size_t write(const uint8_t *buffer, size_t size) override;
    int available(void) override;
    int read(void) override;
    int peek(void) override;
    void flush(void) override;

    size_t read(uint8_t *buffer, size_t size);
    size_t readBytes(char *buffer, size_t length)
    {
        return read(reinterpret_cast<uint8_t *>(buffer), length);
    }

    bool seek(uint32_t pos, SeekMode mode);
    bool seek(uint32_t pos)
    {
        return seek(pos, SeekSet);
    }

    size_t position(void) const;
    size_t size(void) const;
    bool setBufferSize(size_t size);
    void close(void);
    operator bool() const;
    time_t getLastWrite(void) const;
    const char *path(void) const;
    const char *name(void) const;

    bool isDirectory(void) const;
    bool seekDir(long position);
    File openNextFile(const char *mode = FILE_READ);
    String getNextFileName(void);
    String getNextFileName(bool *isDir);
    void rewindDirectory(void);

private:
    void retain(void);
    void release(void);

    FileHandle *handle_;
};

class FS {
public:
    explicit FS(const char *mountPoint = "C:");

    File open(const char *path, const char *mode = FILE_READ, const bool create = false);
    File open(const String &path, const char *mode = FILE_READ, const bool create = false);

    bool exists(const char *path);
    bool exists(const String &path);

    bool remove(const char *path);
    bool remove(const String &path);

    bool rename(const char *pathFrom, const char *pathTo);
    bool rename(const String &pathFrom, const String &pathTo);

    bool mkdir(const char *path);
    bool mkdir(const String &path);

    bool rmdir(const char *path);
    bool rmdir(const String &path);

    const char *mountpoint(void) const;

protected:
    char mountPoint_[4];
    bool ready_;
};

}  // namespace fs

#ifndef FS_NO_GLOBALS
using fs::FS;
using fs::File;
using fs::SeekCur;
using fs::SeekEnd;
using fs::SeekMode;
using fs::SeekSet;
#endif

#endif
