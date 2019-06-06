#include "GstCodec.h"

#ifdef __x86_64__
#define PIPELINE_ENC                                                     \
    "gst-launch-1.0 appsrc name=src ! vaapih264enc ! flvmux ! filesink " \
    "location=%s"
#endif
#ifdef __aarch64__
#define PIPELINE_ENC                                                   \
    "gst-launch-1.0 appsrc name=src ! mpph264enc ! flvmux ! rtmpsink " \
    "location=\"rtmp://127.0.0.1:1935/show/live live=1\" "             \
    "location=%s"
#endif
GstEncoder::GstEncoder(const char *sinkaddr) {
    gst_init(NULL, NULL);

    this->yuv_queue = queue<FrameData>();
    this->bgrSize = height * width * 3;
    this->yuvSize = this->bgrSize / 2 + 1;

    gchar *cmd = g_strdup_printf(PIPELINE_ENC, sinkaddr);

    GError *error = NULL;
    this->pipeline = gst_parse_launch(cmd, &error);

    this->src = gst_bin_get_by_name(GST_BIN(this->pipeline), "src");
    g_object_set(G_OBJECT(this->src), "max-buffers", (guint)20, "block", TRUE,
                 NULL);

    GstCaps *caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING,
                                        "NV12", "height", G_TYPE_INT, height,
                                        "width", G_TYPE_INT, width, NULL);

    gst_app_src_set_caps(GST_APP_SRC(this->src), caps);
    gst_caps_unref(caps);
}

void GstEncoder::pushBuffer(FrameData buffer) {
    this->yuv_mutex.lock();
    this->yuv_queue.push(buffer);
    this->yuv_mutex.unlock();
}
