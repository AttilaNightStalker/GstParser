// #include <glib.h>
// #include <gst/app/app.h>
// #include <gst/app/gstappsink.h>
// #include <gst/gst.h>
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
#include <stdio.h>
#include "libyuv.h"
#include <sys/time.h>

using namespace std;

int main(int argc, char *argv[]) {

    static struct timeval t1,t2;

    gettimeofday(&t1, NULL);
    for (int i = 0; i < 1000000; i++) {
        char *testbuf = new char[10000];
        testbuf[((int)testbuf) % 1000] = 'h';
        // delete testbuf;
    }
    gettimeofday(&t2, NULL);

    printf("time: %lf ms\n", ((double)(t2.tv_sec - t1.tv_sec)*1000.0 + (t2.tv_usec - t1.tv_usec)/1000.0) / 100000);

    sleep(10);
    // int width = 1920;
    // int height = 1080;
    // int size = width*height;

    // int y_stride = width; 
    // int uv_stride = (width + 1) / 2 * 2;
    // int rgb_stride = width * 3;

    // FILE *fp = fopen("sample.yuv", "rb");
    // unsigned char *buffer = new unsigned char[size*4];
    // unsigned char *result = new unsigned char[size*4];

    // fread(buffer, size*4, 1, fp);

    // struct timeval t1, t2;
    // gettimeofday(&t1,NULL);

    // for (int i = 0; i < 1000; i++) {
    //     int res = libyuv::NV12ToRGB24(buffer, y_stride, buffer + size, uv_stride, result, rgb_stride, width, height);
    // }

    // gettimeofday(&t2, NULL);
    // printf("%ld %ld\n", t2.tv_sec, t1.tv_sec);
    // printf("time: %lf ms\n", ((double)(t2.tv_sec - t1.tv_sec)*1000 + ((t2.tv_usec - t1.tv_usec)/1000.0)) / 1000);

    // FILE *wfp = fopen("sample.bgrx", "wb");
    // fwrite(result, size*4, 1, wfp);

    // delete buffer;
    // delete result;
    // fclose(fp);
    // fclose(wfp);

    return 0;
}