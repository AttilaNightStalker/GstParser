#include "FrameConverter.h"
#include "arm_neon.h"

/**/
const uint8_t Y_SUBS[8] = { 16, 16, 16, 16, 16, 16, 16, 16 };
const uint8_t UV_SUBS[8] = { 128, 128, 128, 128, 128, 128, 128, 128 };
void yv12_to_rgb24_neon(int width, int height, char *_src, char *_RGBOut) {

    unsigned char *src = (unsigned char*)_src;
    unsigned char *RGBOut = (unsigned char*)_RGBOut;
    
    int i, j;
    int nWH = width * height;
    unsigned char *pY1 = src;
    unsigned char *pY2 = src + width;
    unsigned char *pUV = src + nWH;

    uint8x8_t Y_SUBvec = vld1_u8(Y_SUBS);
    uint8x8_t UV_SUBvec = vld1_u8(UV_SUBS);

    //int width2 = width >> 1;
    int width3 = (width << 2) - width;
    int width9 = (width << 3) + width;
    unsigned char *RGBOut1 = RGBOut;
    unsigned char *RGBOut2 = RGBOut1 + width3;
    // unsigned char *RGBOut1 = RGBOut + 3 * width * (height - 2);
    // unsigned char *RGBOut2 = RGBOut1 + width3;

    unsigned char tempUV[8];
    // YUV 4:2:0
    for (j = 0; j < height; j += 2) {
        for (i = 0; i < width; i += 8) {
            tempUV[0] = pUV[1];
            tempUV[1] = pUV[3];
            tempUV[2] = pUV[5];
            tempUV[3] = pUV[7];

            tempUV[4] = pUV[0];
            tempUV[5] = pUV[2];
            tempUV[6] = pUV[4];
            tempUV[7] = pUV[6];

            pUV += 8;

            uint8x8_t nUVvec = vld1_u8(tempUV);
            int16x8_t nUVvec16 = vmovl_s8((int8x8_t)vsub_u8(nUVvec, UV_SUBvec));//减后区间-128到127
            int16x4_t V_4 = vget_low_s16((int16x8_t)nUVvec16);
            int16x4x2_t V16x4x2 = vzip_s16(V_4, V_4);
            //int16x8_t V16x8_;
            //memcpy(&V16x8_, &V16x4x2, 16);
            //int16x8_t* V16x8 = (int16x8_t*)(&V16x8_);
            int16x8_t* V16x8 = (int16x8_t*)(&V16x4x2);
            int16x4_t U_4 = vget_high_s16(nUVvec16);
            int16x4x2_t U16x4x2 = vzip_s16(U_4, U_4);

            int16x8_t* U16x8 = (int16x8_t*)(&U16x4x2);

            //公式1
            int16x8_t VV1 = vmulq_n_s16(*V16x8, 102);
            int16x8_t UU1 = vmulq_n_s16(*U16x8, 129);
            int16x8_t VVUU1 = vmlaq_n_s16(vmulq_n_s16(*V16x8, 52), *U16x8, 25);



            uint8x8_t nYvec;
            uint8x8x3_t RGB;
            uint16x8_t Y16;
             //上行
            nYvec = vld1_u8(pY1);
            pY1 += 8;
            //公式1
            Y16 = vmulq_n_u16(vmovl_u8(vqsub_u8(nYvec, Y_SUBvec)), 74);//公式1

            RGB.val[0] = vqmovun_s16(vshrq_n_s16((int16x8_t)vaddq_u16(Y16, (uint16x8_t)UU1), 6));
            RGB.val[1] = vqmovun_s16(vshrq_n_s16((int16x8_t)vsubq_u16(Y16, (uint16x8_t)VVUU1), 6));
            RGB.val[2] = vqmovun_s16(vshrq_n_s16((int16x8_t)vaddq_u16(Y16, (uint16x8_t)VV1), 6));
            vst3_u8(RGBOut1, RGB);
            RGBOut1 += 24;

            //下行
            nYvec = vld1_u8(pY2);
            pY2 += 8;
            //公式1
            Y16 = vmulq_n_u16(vmovl_u8(vqsub_u8(nYvec, Y_SUBvec)), 74);//公式1
            RGB.val[0] = vqmovun_s16(vshrq_n_s16((int16x8_t)vaddq_u16(Y16, (uint16x8_t)UU1), 6));
            RGB.val[1] = vqmovun_s16(vshrq_n_s16((int16x8_t)vsubq_u16(Y16, (uint16x8_t)VVUU1), 6));
            RGB.val[2] = vqmovun_s16(vshrq_n_s16((int16x8_t)vaddq_u16(Y16, (uint16x8_t)VV1), 6));
            vst3_u8(RGBOut2, RGB);
            RGBOut2 += 24;
        }
        RGBOut1 += width3;
        RGBOut2 += width3;
        // RGBOut1 -= width9;
        // RGBOut2 -= width9;
        pY1 += width;
        pY2 += width;
    }
}


void NV12_T_BGR(int width, int height, char *yuyv, char *bgr) {


    const int nv_start = width * height;
    int i, j, index = 0, rgb_index = 0;
    unsigned char y, u, v;
    int r, g, b, nv_index = 0;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            // nv_index = (rgb_index / 2 - width / 2 * ((i + 1) / 2)) * 2;
            nv_index = i / 2 * width + j - j % 2;

            y = yuyv[rgb_index];
            v = yuyv[nv_start + nv_index];
            u = yuyv[nv_start + nv_index + 1];

            r = y + (140 * (v - 128)) / 100;                          // r
            g = y - (34 * (u - 128)) / 100 - (71 * (v - 128)) / 100;  // g
            b = y + (177 * (u - 128)) / 100;                          // b

            if (r > 255) r = 255;
            if (g > 255) g = 255;
            if (b > 255) b = 255;
            if (r < 0) r = 0;
            if (g < 0) g = 0;
            if (b < 0) b = 0;

            index = rgb_index * 3;
            bgr[index + 2] = r;
            bgr[index + 1] = g;
            bgr[index] = b;
            rgb_index++;
        }
    }

}

void NV12_T_BGR_ori(int width, int height, char *_yuyv,
         char *_bgr) {
    unsigned char *yuyv = (unsigned char*)_yuyv;
    unsigned char *bgr = (unsigned char*)_bgr;

    const int nv_start = width * height;
    int i, j, index = 0, rgb_index = 0;
    unsigned char y, u, v;
    int r, g, b, nv_index = 0;

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            //nv_index = (rgb_index / 2 - width / 2 * ((i + 1) / 2)) * 2;
            nv_index = i / 2 * width + j - j % 2;

            y = yuyv[rgb_index];
            v = yuyv[nv_start + nv_index + 1];
            u = yuyv[nv_start + nv_index];
//            u = yuyv[nv_start + nv_index ];
//            v = yuyv[nv_start + nv_index + 1];

            r = y + (140 * (v - 128)) / 100;  //r
            g = y - (34 * (u - 128)) / 100 - (71 * (v - 128)) / 100; //g
            b = y + (177 * (u - 128)) / 100; //b

            if (r > 255)
                r = 255;
            if (g > 255)
                g = 255;
            if (b > 255)
                b = 255;
            if (r < 0)
                r = 0;
            if (g < 0)
                g = 0;
            if (b < 0)
                b = 0;

            // index = rgb_index % width + (height - i - 1) * width;
            // bgr[index * 3 + 2] = r;
            // bgr[index * 3 + 1] = g;
            // bgr[index * 3 + 0] = b;
            // rgb_index++;
            index = rgb_index * 3;
            bgr[index + 2] = r;
            bgr[index + 1] = g;
            bgr[index] = b;
            rgb_index++;
        }
    }

}

void plain(int width, int height, char *in, char *out) {
    int size = width * height * 3 + 1;
    memcpy(out, in, size);
}


void convert(FrameConverter *converter, cpu_set_t cpu) {
    pthread_t cur = pthread_self();
    pthread_setaffinity_np(cur, sizeof(cpu_set_t), &cpu);

    while (true) {
        sem_wait(converter->src_sem);

        converter->src_mutex->lock();
        FrameData &yuv = converter->src->front();
        converter->src->pop();
        converter->src_mutex->unlock();

        int bufSize = yuv.height * yuv.width * 3 + 1;
        char *bgrBuffer = (char *)malloc(bufSize);

        int start = clock();
        converter->method(yuv.width, yuv.height, (char *)yuv.data, (char *)bgrBuffer);
        printf("convert: %lf\n", (double)(clock() - start) / CLOCKS_PER_SEC);

        converter->sink_mutex->lock();
        converter->sink->push(
            FrameData((char *)bgrBuffer, yuv.height, yuv.width, bufSize));
        sem_post(converter->sink_sem);
        converter->sink_mutex->unlock();

        delete yuv.data;
        dprintf("pulled yuv\n");
    }
}

FrameConverter::FrameConverter(FrameType origin, FrameType target,
                               MutexQueue src_queue, MutexQueue sink_queue) {
    this->src = src_queue.que;
    this->src_mutex = src_queue.mtx;
    this->src_sem = src_queue.sem;
    this->sink = sink_queue.que;
    this->sink_mutex = sink_queue.mtx;
    this->sink_sem = sink_queue.sem;

    switch (origin) {
        case NV12:
            switch (target) {
                case NV12:
                    g_error(
                        "FrameConverter: nothing to convert from NV12 to "
                        "NV12\n");
                    break;

                case BGR:
                    this->method = plain;//yv12_to_rgb24_neon;
                    break;
                case YUV422:
                    g_error("invalid conversion: NV12 to YUV422\n");
                default:
                    break;
            }
            break;

        default:
            g_error("unknow conversion type\n");
            exit(0);
    }
}


void FrameConverter::run(cpu_set_t cpu) {
    thread t = thread(convert, this, cpu);
    t.detach();
}
