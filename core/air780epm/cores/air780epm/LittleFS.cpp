#include "LittleFS.h"

extern "C" {
int32_t getFsTotalSize(uint8_t partation);
int32_t getFsFreeSize(uint8_t partation);
int LFS_init(void);
int LFS_format(void);
}

namespace fs {

static const uint8_t kFlashPartitionC = 0U;

LittleFSClass::LittleFSClass()
    : FS("C:")
    , mounted_(false)
    , runtimeInitialized_(false)
{
    ready_ = false;
}

bool LittleFSClass::begin(bool formatOnFail,
                          const char *basePath,
                          uint8_t maxOpenFiles,
                          const char *partitionLabel)
{
    (void)basePath;
    (void)maxOpenFiles;
    (void)partitionLabel;

    if (!runtimeInitialized_) {
        if (LFS_init() != 0) {
            mounted_ = false;
            ready_ = false;
            return false;
        }
        runtimeInitialized_ = true;
    }

    if (totalBytes() > 0U) {
        mounted_ = true;
        ready_ = true;
        return true;
    }

    if (formatOnFail && format()) {
        mounted_ = totalBytes() > 0U;
    } else {
        mounted_ = false;
    }
    ready_ = mounted_;

    return mounted_;
}

bool LittleFSClass::format(void)
{
    if (!runtimeInitialized_) {
        if (LFS_init() != 0) {
            return false;
        }
        runtimeInitialized_ = true;
    }

    return LFS_format() == 0;
}

size_t LittleFSClass::totalBytes(void)
{
    int32_t total = getFsTotalSize(kFlashPartitionC);

    if (total < 0) {
        return 0U;
    }

    return (size_t)total;
}

size_t LittleFSClass::usedBytes(void)
{
    int32_t total = getFsTotalSize(kFlashPartitionC);
    int32_t freeBytes = getFsFreeSize(kFlashPartitionC);

    if (total < 0 || freeBytes < 0 || total < freeBytes) {
        return 0U;
    }

    return (size_t)(total - freeBytes);
}

void LittleFSClass::end(void)
{
    mounted_ = false;
    ready_ = false;
}

bool LittleFSClass::mounted(void) const
{
    return mounted_;
}

}  // namespace fs

fs::LittleFSClass LittleFS;
