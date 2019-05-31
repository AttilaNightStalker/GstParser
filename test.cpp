#include <glib.h>
// #include <gst/app/app.h>
// #include <gst/app/gstappsink.h>
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

using namespace std;

#define TEST_WAIT 1

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

void run_loop(GMainLoop *loop) {
    // printf("run\n");
    g_main_loop_run(loop);
    // g_main_loop_run (loop);
}

int main(int argc, char *argv[]) {
    gst_init(NULL, NULL);
    gchar *cmd =
        g_strdup_printf("gst-launch-1.0 v4l2src device=/dev/video%d ! tee name=t ! queue ! videoconvert ! mpph264enc ! h264parse ! flvmux ! rtmpsink location=\"rtmp://127.0.0.1:1935/show/live live=1\" t. ! fakesink", atoi(argv[1]));

        // g_strdup_printf("gst-launch-1.0 v4l2src device=/dev/video%d ! tee name=t ! queue ! videoconvert ! autovideosink t. ! queue ! fakesink", atoi(argv[1]));

    GError *error = NULL;
    GstElement *pipeline = gst_parse_launch(cmd, &error);

    bool success = false;
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    guint bus_watch = gst_bus_add_watch(bus, test_bus_callback, &success);
    gst_object_unref(bus);

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);;
    thread t = thread(run_loop, loop);
    
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    t.join();
    sleep(TEST_WAIT);
}