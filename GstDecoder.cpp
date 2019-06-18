#include "GstCodec.h"

// #define TEST

#ifdef TEST
#define HEIGHT 576
#define WIDTH 704
#endif  // TEST



void saveFrame(GstElement *sink, MutexQueue *queue_mtx) {

    static int lost = 0, total = 0, time;
    static struct timeval t1,t2;

    gettimeofday(&t2, NULL);
    printf("seg time: %lf ms\n", (double)(t2.tv_sec - t1.tv_sec)*1000.0 + (t2.tv_usec - t1.tv_usec)/1000.0);

    gettimeofday(&t1,NULL);

    dprintf("saving frame, size: %d\n", (int)queue_mtx->que->size());
    if (queue_mtx->que->size() >= 10) {
        lost++;
        total++;

        GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
        gst_sample_unref(sample);

        printf("************************************************************\n");
        printf("lost rate: %lf\n", (double)lost / total);
        printf("************************************************************\n");
        queue_mtx->mtx->unlock();

        //free(queue_mtx);
        return;
    }
    queue_mtx->mtx->unlock();
    total++;

    int height, width;

    GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
    if (sample == NULL) {printf("pull sample failed when saving a frame in the call back function: GstDecoder.cpp, line44\n"); exit(1);}

    GstBuffer *buffer = gst_sample_get_buffer(sample);
    if (buffer == NULL) {printf("pull buffer failed when saving a frame in the call back function: GstDecoder.cpp, line47\n"); exit(1);}

    GstMemory *memory = gst_buffer_get_all_memory(buffer);
    if (memory == NULL) {printf("pull memory failed when saving a frame in the call back function: GstDecoder.cpp, line50\n"); exit(1);}


    GstMapInfo map;
    if (!gst_memory_map(memory, &map, GST_MAP_READ)) {
        gst_memory_unref(memory);
        gst_sample_unref(sample);
        printf("map failed\n");
        return;
    }

    GstCaps *caps = gst_sample_get_caps(sample);
    GstStructure *structure = gst_caps_get_structure(caps, 0);

    gst_structure_get_int(structure, "height", &height);
    gst_structure_get_int(structure, "width", &width);

    int size = width*height;
    int y_stride = width; 
    int uv_stride = (width + 1) / 2 * 2;
    int rgb_stride = width * 3;

    unsigned char *dumpbuffer = (unsigned char *)malloc(size*3 + 1);

#ifndef RGA
    unsigned char *readbuffer = (unsigned char *)malloc(map.size);
    memcpy(readbuffer, map.data, map.size);
    libyuv::NV12ToRGB24(readbuffer, y_stride, readbuffer + size, uv_stride, dumpbuffer, rgb_stride, width, height);
    delete readbuffer;
#else
    memcpy(dumpbuffer, map.data, map.size);
#endif

    printf("map.size: %ld\n", map.size);

    queue_mtx->mtx->lock();
    /*locked*/
    queue_mtx->que->push(FrameData((char*)dumpbuffer, height, width, size*3 + 1));
    sem_post(queue_mtx->sem);
    /*unlocked*/
    queue_mtx->mtx->unlock();

    // pthread_mutex_unlock(&queue_m);
    dprintf("pushed yuv, bufsize %d\n", (int)map.size);

    gst_memory_unmap(memory, &map);
    gst_memory_unref(memory);
    // gst_buffer_unref(buffer);
    gst_sample_unref(sample);
    
    gettimeofday(&t2, NULL);
    printf("time: %lf ms\n", (double)(t2.tv_sec - t1.tv_sec)*1000.0 + (t2.tv_usec - t1.tv_usec)/1000.0);
    // free(queue_mtx);

    gettimeofday(&t1,NULL);

    return;
}


void splitString(string &str, vector<string> &strvec) {
    for (int i = 0, j = 0; j < str.length(); j++) {
        if (str[j] == '\n') {
            strvec.push_back(str.substr(i, j - i));
            i = j + 1;
        }
    }
}

gboolean test_bus_callback(GstBus *bus, GstMessage *message, gpointer data) {
    bool *isSuccess = (bool *)data;
    printf("Got %s message\n", GST_MESSAGE_TYPE_NAME(message));

    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_ERROR:
            GError *err;
            gchar *debug;

            gst_message_parse_error(message, &err, &debug);
            printf("Error: %s\n", err->message);
            g_error_free(err);
            g_free(debug);
            break;

        case GST_MESSAGE_STATE_CHANGED:
            GstState oldState, newState;
            gst_message_parse_state_changed(message, &oldState, &newState,
                                            NULL);
            printf("old state: %s, new state %s\n",
                    gst_element_state_get_name(oldState),
                    gst_element_state_get_name(newState));

            if (newState == GST_STATE_PLAYING) {
                *isSuccess = true;
            }
        default:
            break;
    }

    return TRUE;
}

gboolean run_bus_callback(GstBus *bus, GstMessage *message, gpointer data) {
    GstDecoder *cur = (GstDecoder*)data;
    printf("Got %s message\n", GST_MESSAGE_TYPE_NAME(message));

    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_ERROR:
            GError *err;
            gchar *debug;

            gst_message_parse_error(message, &err, &debug);
            printf("Error: %s\n", err->message);
            g_error_free(err);
            g_free(debug);
            
            if (cur->type == USB_CAM) {
                while (!cur->reset()) {
                    sleep(2);
                }
            }
            break;

        case GST_MESSAGE_STATE_CHANGED:
            GstState oldState, newState;
            gst_message_parse_state_changed(message, &oldState, &newState,
                                            NULL);
            printf("old state: %s, new state %s\n",
                    gst_element_state_get_name(oldState),
                    gst_element_state_get_name(newState));
        
        // case GST_MESSAGE_STREAM_STATUS:
        //     GstStreamStatusType type;
        //     gst_message_parse_stream_status(message, &type, NULL);
        //     printf("stream status type: %d\n", type);

        default:
            break;
    }

    return TRUE;
}

void run_loop(GMainLoop *loop) {
    // printf("run\n");
    g_main_loop_run(loop);
    // g_main_loop_run (loop);
}


char *usbTest(int videono) {
    char buffer[128];
    vector<string> videos = vector<string>();

    string result = "";
    const char *search = "ls /dev/ | grep video";

    FILE *pipe = popen(search, "r");
    if (!pipe) throw runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }

    printf("** %s **\n", result.c_str());
    splitString(result, videos);

    if (videos.size() <= videono) {
        g_error("wrong video number\n");
        return NULL;
    }

    int i, cnt = 0;
    bool success;
    for (i = 0; i < videos.size() && cnt <= videono; i++) {
        success = false;
        if (cnt + videos.size() - i <= videono) {
            g_error("Can't load the camera\n");
            return NULL;
        }
        gchar *cmd =
            g_strdup_printf("gst-launch-1.0 v4l2src device=/dev/%s ! fakesink",
                            videos[i].c_str());

        GError *error = NULL;
        GstElement *testpipe = gst_parse_launch(cmd, &error);

        // success = false;
        // GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(testpipe));
        // guint bus_watch = gst_bus_add_watch(bus, test_bus_callback, &success);
        // gst_object_unref(bus);

        GstStateChangeReturn ret = gst_element_set_state(testpipe, GST_STATE_PLAYING);
        sleep(TEST_WAIT);

        GstState state, pending;
        ret = gst_element_get_state(testpipe, &state, &pending, GST_CLOCK_TIME_NONE);
        success = (state == GST_STATE_PLAYING);
        printf("state: %d\n", (int)state);

        gst_element_set_state(testpipe, GST_STATE_NULL);
        // g_source_remove(bus_watch);
        // gst_object_unref(testpipe);
        cnt = cnt + 1 ? success : cnt;
    }

    if (!success) {
        return NULL;
    }

    char *ret = new char[videos[i - 1].size()];
    memcpy(ret, videos[i - 1].c_str(), videos[i - 1].size());
    return ret;
}

bool rtspTest(const char *src) {
    gst_init(NULL, NULL);

    gchar *cmd = g_strdup_printf("rtspsrc location=%s ! fakesink", src);
    GError *error = NULL;
    GstElement *testpipe = gst_parse_launch(cmd, &error);

    GstStateChangeReturn ret = gst_element_set_state(testpipe, GST_STATE_PLAYING);
    sleep(TEST_WAIT);

    GstState state, pending;
    ret = gst_element_get_state(testpipe, &state, &pending, GST_CLOCK_TIME_NONE);

    printf("state: %d\n", (int)state);
    return state == GST_STATE_PLAYING;
}

/********************************************************************************************************************/
/* IP camera source
/********************************************************************************************************************/

#ifdef __x86_64__
    #define PIPELINE_DEC                                                   \
        "gst-launch-1.0 rtspsrc location=%s ! rtph264depay ! h264parse ! " \
        "decodebin ! videoconvert ! video/x-raw,format=NV12 ! appsink name=sink "
#endif

#ifdef __aarch64__
    #ifndef RGA
        #define PIPELINE_DEC                                                         \
            "gst-launch-1.0 rtspsrc location=%s ! rtph264depay ! h264parse ! tee name=t ! queue ! decodebin caps='video/x-raw,format=NV21' ! appsink name=sink name=sink t. ! queue ! flvmux ! rtmpsink location=%s"    
    #else
        #define PIPELINE_DEC "gst-launch-1.0 rtspsrc location=%s ! rtph264depay ! h264parse ! tee name=t ! queue ! decodebin ! rgaconvert ! video/x-raw,format=BGRA ! appsink name=sink t. ! queue ! flvmux ! rtmpsink location=%s" 
    #endif
#endif

GstDecoder::GstDecoder(const char *src, const char *dst) {
    gst_init(NULL, NULL);

    this->type = RTSP_STREAM;
    this->srcdev.srcurl = (char*)malloc(100);
    sprintf(this->srcdev.srcurl, "%s", src);
    this->dstaddr = (char*)malloc(100);
    sprintf(this->dstaddr, "%s", dst);
    this->bgr_queue = queue<FrameData>();
    sem_init(&this->bgr_sem, 0, 0);

    gchar *cmd = g_strdup_printf(PIPELINE_DEC, src, dst);

    GError *error = NULL;
    this->pipeline = gst_parse_launch(cmd, &error);
    // g_assert(error == NULL);

    bool success = false;
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    guint bus_watch = gst_bus_add_watch(bus, run_bus_callback, &success);
    gst_object_unref(bus);

    this->loop = g_main_loop_new(NULL, FALSE);
    thread t = thread(run_loop, loop);
    t.detach();

    this->sink = gst_bin_get_by_name(GST_BIN(this->pipeline), "sink");
    g_object_set(G_OBJECT(this->sink), "emit-signals", TRUE, "sync", FALSE,
                 "max-buffers", (guint)10, NULL);

    g_signal_connect(
        this->sink, "new-sample", G_CALLBACK(saveFrame),
        new MutexQueue(&this->bgr_mutex, &this->bgr_sem, &this->bgr_queue));
}



/********************************************************************************************************************/
/* USB camera source
/********************************************************************************************************************/

#ifdef __x86_64__
#define PIPELINE_CMD                                                \
    "gst-launch-1.0 v4l2src device=/dev/%s ! tee name=t ! queue ! " \
    " appsink name=sink t. ! queue "                                \
    "! videorate caps=\"video/x-raw,framerate=20/1\" ! autovideosink"
#endif
// #ifdef __aarch64__
// #define PIPELINE_CMD                                                   \
//     "gst-launch-1.0 v4l2src name=src device=/dev/%s ! queue ! tee name=t ! queue ! videorate ! video/x-raw,framerate=15/1 ! appsink name=sink t. ! queue ! mpph264enc ! h264parse ! flvmux ! rtmpsink location=\"rtmp://127.0.0.1:1935/show/live live=1\" "
// #endif

#ifdef __aarch64__
#ifndef RGA
#define PIPELINE_CMD "gst-launch-1.0 v4l2src name=src device=/dev/%s ! queue ! tee name=t ! queue ! mpph264enc ! h264parse ! flvmux ! rtmpsink location=%s t. ! queue ! videoconvert ! video/x-raw,format=NV12,framerate=20/1 ! appsink name=sink"
#else
#define PIPELINE_CMD "gst-launch-1.0 v4l2src name=src device=/dev/%s ! queue ! tee name=t ! queue ! mpph264enc ! h264parse ! flvmux ! rtmpsink location=%s t. ! queue ! videoconvert ! video/x-raw,format=NV12,framerate=20/1 ! rgaconvert ! video/x-raw,format=BGR ! queue ! appsink name=sink"
#endif
#endif

GstDecoder::GstDecoder(int videono, const char* dst) {
    gst_init(NULL, NULL);
    this->type = USB_CAM;
    this->srcdev.videono = videono;
    this->dstaddr = (char*)malloc(100);
    sprintf(this->dstaddr, "%s", dst);
    this->loop = g_main_loop_new(NULL, FALSE);
    thread t = thread(run_loop, loop);
    t.detach();

    char *vidaddr = usbTest(this->srcdev.videono);
    if (vidaddr == NULL) {
        g_error("Can't find video\n");
        exit(0);
    }

    this->bgr_queue = queue<FrameData>();
    sem_init(&this->bgr_sem, 0, 0);

    gchar *cmd = g_strdup_printf(PIPELINE_CMD, vidaddr, dst);

    /* set src frame rate*/
    //GstElement *v4l2src = gst_bin_get_by_name(GST_BIN(this->pipeline), "src");
    //g_object_set(GST_OBJECT(v4l2src),"framerate","15/1",NULL);
    // g_object_unref(v4l2src);

    GError *error = NULL;
    this->pipeline = gst_parse_launch(cmd, &error);

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(this->pipeline));
    this->watch = gst_bus_add_watch(bus, run_bus_callback, this);
    gst_object_unref(bus);

    this->sink = gst_bin_get_by_name(GST_BIN(this->pipeline), "sink");
    g_object_set(G_OBJECT(this->sink), "emit-signals", TRUE, "sync", FALSE,
                 "max-buffers", (guint)10, NULL);

    g_signal_connect(
        this->sink, "new-sample", G_CALLBACK(saveFrame),
        new MutexQueue(&this->bgr_mutex, &this->bgr_sem, &this->bgr_queue));
}


bool GstDecoder::reset() {
    if (this->pipeline != NULL) {
        GstStateChangeReturn ret = gst_element_set_state(this->pipeline, GST_STATE_NULL);                   
    }
    printf("reseting\n");
    // if (this->loop != NULL) {
    //     //g_main_loop_unref(loop);
    //     this->loop = NULL;
    // }

    char *vidaddr = usbTest(this->srcdev.videono);
    if (vidaddr == NULL) {
        printf("\n Reset failed\n\n");
        return false;
    }

    GError *error = NULL;
    gchar *cmd = g_strdup_printf(PIPELINE_CMD, vidaddr, this->dstaddr);
    this->pipeline = gst_parse_launch(cmd, &error);

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(this->pipeline));
    guint bus_watch = gst_bus_add_watch(bus, run_bus_callback, this);
    gst_object_unref(bus);

    // this->loop = g_main_loop_new(NULL, FALSE);
    // thread t = thread(run_loop, this->loop);
    // t.detach();

    this->sink = gst_bin_get_by_name(GST_BIN(this->pipeline), "sink");
    g_object_set(G_OBJECT(this->sink), "emit-signals", TRUE, "sync", FALSE,
                 "max-buffers", (guint)10, NULL);

    g_signal_connect(
        this->sink, "new-sample", G_CALLBACK(saveFrame),
        new MutexQueue(&this->bgr_mutex, &this->bgr_sem, &this->bgr_queue));
    

    GstStateChangeReturn ret = gst_element_set_state(this->pipeline, GST_STATE_PLAYING);
    printf("reset ret: %d\n", (int)ret);
    printf("\n Reset succeeded\n\n");
    return true;
}

GstStateChangeReturn GstDecoder::setPlay(cpu_set_t &cpu, bool convert) {}

GstStateChangeReturn GstDecoder::setPlay(cpu_set_t &cpu) {
    // this->convertYUV.detach();
    //this->frameCvt.run(cpu);
    if (this->type = RTSP_STREAM) {
        while (!rtspTest(this->srcdev.srcurl));
    }
    
    return gst_element_set_state(this->pipeline, GST_STATE_PLAYING);
}

GstStateChangeReturn GstDecoder::setPause() {
    return gst_element_set_state(this->pipeline, GST_STATE_PAUSED);
}

FrameData GstDecoder::getFrame() {
    sem_wait(&this->bgr_sem);

    this->bgr_mutex.lock();
    dprintf("get a frame\n");
    FrameData frame = this->bgr_queue.front();

    this->bgr_queue.pop();
    this->bgr_mutex.unlock();

    return frame;
}
