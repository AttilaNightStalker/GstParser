#include "FrameConverter.h"

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

    GstElement *pipeline, *sink;
    GMainLoop *loop;
    queue<FrameData> yuv_queue;
    queue<FrameData> bgr_queue;

    guint watch;

    mutex yuv_mutex;
    mutex bgr_mutex;
    sem_t bgr_sem;
    sem_t yuv_sem;


   public:
    int type;
    
    GstDecoder(const char* src);
    GstDecoder(int videoNo);

    GstStateChangeReturn setPlay(cpu_set_t &cpu);
    GstStateChangeReturn setPlay(cpu_set_t &cpu, bool convert);

    GstStateChangeReturn setPause();
    FrameData getFrame();
    FrameData getRaw();

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


/*
gst-launch-1.0 rtspsrc
location=rtsp://admin:IVA_1114@192.168.10.65:554/h264/ch1/sub/av_stream !
rtph264depay ! h264parse ! decodebin caps=\'video/x-raw,format-NV12\' !
mpph264enc ! h264parse
*/