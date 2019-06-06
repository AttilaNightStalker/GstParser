#include <glib.h>
#include <gst/app/app.h>
#include <gst/app/gstappsink.h>
#include <gst/gst.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <cstring>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>
#include <sys/time.h>

#include "libyuv.h"

#define dprintf

using namespace std;

class FrameConverter;

struct ConvertTable {
    int crv[256];
    int cbu[256];
    int cgu[256];
    int cgv[256];
    int tab[156];
    unsigned char clp[1024];
    ConvertTable();
};

struct FrameData {
    char *data;
    int height, width, size;
    FrameData(char *_data, int _height, int _width, int _size)
        : data(_data), height(_height), width(_width), size(_size) {}
};

struct MutexQueue {
    mutex *mtx;
    sem_t *sem;
    queue<FrameData> *que;
    MutexQueue(mutex *_mtx, sem_t *_sem, queue<FrameData> *_que)
        : mtx(_mtx), sem(_sem), que(_que) {}
};

enum FrameType { BGR, NV12, YUV422 };

class FrameConverter {
   private:
    void (*method)(int width, int height, char *origin, char *target);
    friend void convert(FrameConverter *converter, cpu_set_t cpu);
    thread converting;
    void *convertTable;

    queue<FrameData> *src;
    queue<FrameData> *sink;

    mutex *src_mutex;
    mutex *sink_mutex;

    sem_t *src_sem;
    sem_t *sink_sem;

    int height, width;

   public:
    FrameConverter(FrameType origin, FrameType target, MutexQueue src_queue,
                   MutexQueue sink_queue);
    FrameConverter(){}
    void run(cpu_set_t cpu);
};
