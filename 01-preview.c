#include <stdio.h>
#include <stdlib.h>
#include <gst/gst.h>

int main(int argc, char **argv)
{
    gst_init(&argc, &argv);
    gboolean gRet = FALSE;
    printf("[DEBUG] Gstreamer initialised\n");


    GstElement *pipeline = gst_pipeline_new("pipeline");
    if (NULL == pipeline) {
        printf("[ERROR] Couldn't create pipeline\n");
        return -1;
    }

    GstElement *src = gst_element_factory_make("v4l2src", "video source");
    if (NULL == src) {
        printf("[ERROR] Couldn't create source pipeline element\n");
        return -1;
    }

    GstElement *convert = gst_element_factory_make("videoconvert", "video converter");
    if (NULL == convert) {
        printf("[ERROR] Couldn't create video converter element\n");
        return -1;
    }

    GstElement *screen = gst_element_factory_make("autovideosink", "video destination");
    if (NULL == screen) {
        printf("[ERROR] Couldn't create output element\n");
        return -1;
    }

    g_object_set(src, "device", "/dev/video0", NULL);
    printf("[DEBUG] Linked video device with source pipeline element");

    gst_bin_add_many(GST_BIN(pipeline), src, convert, screen, NULL);
    gRet = gst_element_link_many(src, convert, screen, NULL);
    if (FALSE == gRet) {
        printf("[ERROR] Couldn't link elements in pipeline\n");
        gst_object_unref(pipeline);
        return -1;
    }

     if (gst_element_set_state(pipeline, GST_STATE_PLAYING)
            == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Pornire esuata.\n");
        gst_object_unref(pipeline);
        return -1;
    }
    g_print("Rulez. Ctrl+C pentru oprire.\n");
    
    GstBus *bus = gst_element_get_bus(pipeline);
    GstMessage *msg = gst_bus_timed_pop_filtered(
        bus, GST_CLOCK_TIME_NONE,
        (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    if (msg) {
        if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
            GError *e = NULL;
            gchar *dbg = NULL;
            gst_message_parse_error(msg, &e, &dbg);
            g_printerr("Eroare de la %s: %s\n",
                       GST_OBJECT_NAME(msg->src), e->message);
            g_printerr("[DEBUG]: %s\n", dbg ? dbg : "(no details)");
            g_clear_error(&e);
            g_free(dbg);
        }
        gst_message_unref(msg);
    }

    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return 0;

}