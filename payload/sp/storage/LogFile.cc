extern "C" {
#include "LogFile.h"
}

#include "sp/ScopeLock.hh"

#include <cstdio>

namespace SP::LogFile {

static const size_t BUFFER_SIZE = 4096;
static bool isInit = false;
static OSTime startTime;
static File file;
static char buffers[BUFFER_SIZE][2];
static u8 index = 0;
static u16 offset = 0;

static void Init() {
    startTime = OSGetTime();

    if (!Storage_open(&file, L"/mkw-sp/log.txt", "w")) {
        return;
    }

    isInit = true;
}

static void VPrintf(const char *format, va_list args) {
    if (!isInit) {
        return;
    }

    ScopeLock<NoInterrupts> lock;

    u16 maxLength = BUFFER_SIZE - offset;
    OSTime currentTime = OSGetTime();
    u32 secs = OSTicksToSeconds(currentTime - startTime);
    u32 msecs = OSTicksToMilliseconds(currentTime - startTime) % 1000;
    u32 length = snprintf(buffers[index] + offset, maxLength, "[%lu.%03lu] ", secs, msecs);
    if (length >= maxLength) {
        offset = BUFFER_SIZE;
        return;
    }
    offset += length;
    maxLength -= length;
    length = vsnprintf(buffers[index] + offset, maxLength, format, args);
    if (length >= maxLength) {
        offset = BUFFER_SIZE;
        return;
    }
    offset += length;
}

static void Flush() {
    if (!isInit) {
        return;
    }

    u8 oldIndex;
    u16 oldOffset;
    {
        ScopeLock<NoInterrupts> lock;

        if (offset == 0) {
            return;
        }

        oldIndex = index;
        index ^= 1;

        oldOffset = offset;
        offset = 0;
    }

    Storage_write(&file, buffers[oldIndex], oldOffset, Storage_size(&file));
    Storage_sync(&file);
}

} // namespace SP::LogFile

extern "C" {
void LogFile_Init(void) {
    SP::LogFile::Init();
}

void LogFile_VPrintf(const char *format, va_list args) {
    SP::LogFile::VPrintf(format, args);
}

void LogFile_Flush(void) {
    SP::LogFile::Flush();
}
}
