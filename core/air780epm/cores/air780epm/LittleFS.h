#ifndef LITTLEFS_H
#define LITTLEFS_H

#include "FS.h"

namespace fs {

class LittleFSClass : public FS {
public:
    LittleFSClass();

    bool begin(bool formatOnFail = false,
               const char *basePath = NULL,
               uint8_t maxOpenFiles = 10,
               const char *partitionLabel = NULL);
    bool format(void);
    size_t totalBytes(void);
    size_t usedBytes(void);
    void end(void);
    bool mounted(void) const;

private:
    bool mounted_;
    bool runtimeInitialized_;
};

}  // namespace fs

extern fs::LittleFSClass LittleFS;

#endif
