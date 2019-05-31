#include "GstCodec.h"
#include <iostream>

using namespace std;

// #define RTSP_SRC "rtsp://admin:IVA_1114@192.168.10.65:554/mpeg4" //h264/ch1/sub/av_stream"
#define RTSP_SRC "rtsp://admin:object2018@192.168.10.111:554/h264/ch1/main/av_stream"
#define RTSP_FRT "rtsp://admin:ad123456@192.168.10.67:554/mpeg4"


int main() {
#ifdef __aarch64__
    printf("aarch64\n");
#endif
    GstDecoder decoder(RTSP_SRC);
    cpu_set_t cpu;
    CPU_ZERO(&cpu);
    CPU_SET(1, &cpu);
    decoder.setPlay(cpu);
    // char *buffer = new char[BUF_SIZE];
    // decoder.getFrame(buffer);


    while (true) {
        usleep(40);
        FrameData bgr = decoder.getFrame();
        if (bgr.size == 0) {
            continue;
        }
        delete bgr.data;
        dprintf("got a frame\n");
        // FILE* fd = fopen("sample.bgr", "wb");
        // fwrite(bgr.data, sizeof(char), bgr.size, fd);
        // printf("%d, %d\n", bgr.height, bgr.width);
        // break;
    }

    // decoder.setPause();
}
