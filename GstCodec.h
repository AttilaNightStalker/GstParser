#include "FrameConverter.h"

/*
* if RGA device exists uncomment this
*/
#define RGA

#ifndef RGA
#include "libyuv.h"
#endif

/*
* for official compilation, comment this
*/
#define DEBUG

#ifndef DEBUG
    #define printf
#endif

#define SETCPU1 0
#define SETCPU2 1
#define MAX_QUEUE 10
#define TEST_WAIT 2

#define USB_CAM 0
#define RTSP_STREAM 1

using namespace std;


bool rtspTest(const char *src);


class GstDecoder {
   private:
    union {
        int videono;
        char *srcurl;
    } srcdev;

    char *dstaddr;

    GstElement *pipeline, *sink;
    GMainLoop *loop;
    queue<FrameData> bgr_queue;

    guint watch;

    mutex bgr_mutex;
    sem_t bgr_sem;

   public:
    int type;
    
    GstDecoder(const char *src, const char *dst);
    GstDecoder(int videono, const char* dst);

    GstStateChangeReturn setPlay(cpu_set_t &cpu);
    GstStateChangeReturn setPlay(cpu_set_t &cpu, bool convert);

    GstStateChangeReturn setPause();
    FrameData getFrame();

    bool reset();
};


class GstEncoder {
   private:
    mutex yuv_mutex;
    queue<FrameData> yuv_queue;
    GstElement *pipeline, *src;
    int height, width;
    int bgrSize, yuvSize;

   public:
    GstEncoder(const char *sinkaddr);
    void pushBuffer(FrameData buffer);
};

